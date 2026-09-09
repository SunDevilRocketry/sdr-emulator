/**
* @file: emulator_lora.c
*
* Mocks the functionality of LoRa on the FC.
*                                                                             
*
* @copyright:                                                                  
*       Copyright (c) 2026 Sun Devil Rocketry.                                
*       All rights reserved.                                                  
*                                                                             
*       This software is licensed under terms that can be found in the LICENSE
*       file in the root directory of this software component.                 
*       If no LICENSE file comes with this software, it is covered under the   
*       BSD-3-Clause.                                                          
*                                                                              
*       https://opensource.org/license/bsd-3-clause                            
*
*/

/*------------------------------------------------------------------------------
 Includes                                                         
------------------------------------------------------------------------------*/
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <stddef.h>
#include <pthread.h>
#include <unistd.h>

#include "emulator.h"
#include "main.h"
#include "stm32h7xx_hal.h"
#include "sdr_pin_defines_A0002.h"
#include "lora.h"

/*------------------------------------------------------------------------------
 Statics
------------------------------------------------------------------------------*/
typedef enum {
    EMULATOR_LORA_SPI_IDLE = 0,
    EMULATOR_LORA_SPI_READ_REG,
    EMULATOR_LORA_SPI_WRITE_REG
} EMULATOR_LORA_SPI_STATE;

typedef enum {
    EMULATOR_LORA_EVENT_NONE = 0,
    EMULATOR_LORA_EVENT_DMA_CPLT
} EMULATOR_LORA_EVENT;

typedef struct {
    bool initialized;
    bool tx_pending;
    uint8_t registers[256];
    uint8_t last_payload[256];
    size_t last_payload_len;
    uint32_t last_tx_time_ms;
    uint8_t pending_reg;
    bool pending_reg_write;
    EMULATOR_LORA_SPI_STATE spi_state;
    bool dma_op_is_tx; /* which async DMA op emulator_lora_it_listener() should complete */

    uint8_t fifo[256];
    uint8_t fifo_len;
    uint8_t fifo_tx_base_addr;
    uint8_t fifo_spi_pointer;
    uint8_t irq_flags;
    EMULATOR_LORA_EVENT pending_event;
} EMULATOR_LORA_STATE;

static EMULATOR_LORA_STATE emulator_lora_state = {0};
static pthread_mutex_t emulator_lora_it_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t emulator_lora_it_cond = PTHREAD_COND_INITIALIZER;
static pthread_t emulator_lora_it_thread;
static bool emulator_lora_it_thread_started = false;

/*------------------------------------------------------------------------------
 Procedure prototypes                                                        
------------------------------------------------------------------------------*/
static void emulator_lora_reset_state(void);
static void emulator_lora_set_register(uint8_t reg, uint8_t value);
static void emulator_lora_handle_payload(const uint8_t *data, size_t len);
static void emulator_lora_store_fifo_bytes(const uint8_t *data, size_t len);
static void emulator_lora_execute_tx(void);
static void emulator_lora_schedule_completion(EMULATOR_LORA_EVENT event);
static void emulator_lora_start_thread_if_needed(void);
static void *emulator_lora_it_listener(void *arg);
static void lora_ota_transmit(void *data, size_t len, uint32_t time_to_tx_ms);
uint32_t emulator_lora_spi_transmit_dma(void *hspi, const uint8_t *pData, uint16_t Size);
uint32_t emulator_lora_spi_transmit_receive_dma(void *hspi, const uint8_t *pTxData, uint8_t *pRxData, uint16_t Size);

/*------------------------------------------------------------------------------
 HAL interfaces                                                         
------------------------------------------------------------------------------*/

/**
 * @brief Top level mock function for LoRa reg reads.
 * 
 * @param hspi (ignored)
 * @param pTxData The transmit buffer
 * @param pRxData The receive buffer
 * @param Size Size of the data to TX/RX
 * @param Timeout (ignored)
 * @return HAL_StatusTypeDef 
 */
uint32_t emulator_lora_spi_transmit_receive
    (
    void *hspi, 
    const uint8_t *pTxData, 
    uint8_t *pRxData, 
    uint16_t Size, 
    uint32_t Timeout
    )
{
(void)hspi;
(void)Timeout;

if (pTxData == NULL || pRxData == NULL || Size == 0U) {
    return HAL_OK;
}

if (!emulator_lora_state.initialized) {
    emulator_lora_reset_state();
    emulator_lora_state.initialized = true;
}

emulator_lora_start_thread_if_needed();

if ((pTxData[0] & 0x80U) == 0U) {
    const uint8_t reg = (uint8_t)(pTxData[0] & 0x7FU);
    memset(pRxData, 0x00U, Size);

    if (Size >= 2U) {
        pRxData[1] = emulator_lora_state.registers[reg];
    }
    else {
        pRxData[0] = emulator_lora_state.registers[reg];
    }

    emulator_lora_state.pending_reg = reg;
    emulator_lora_state.pending_reg_write = false;
    emulator_lora_state.spi_state = EMULATOR_LORA_SPI_IDLE;
    return HAL_OK;
}

if (Size >= 2U) {
    const uint8_t reg = (uint8_t)(pTxData[0] & 0x7FU);
    memset(pRxData, 0x00U, Size);
    emulator_lora_set_register(reg, pTxData[1]);
    emulator_lora_state.pending_reg = reg;
    emulator_lora_state.pending_reg_write = false;
    emulator_lora_state.spi_state = EMULATOR_LORA_SPI_IDLE;
    return HAL_OK;
}

memset(pRxData, 0x00U, Size);
return HAL_OK;

} /* emulator_lora_spi_transmit_receive */


/**
 * @brief Top level mock function for LoRa.
 * 
 * @param hspi SPI handle (ignored)
 * @param pData Pointer to the data to transmit
 * @param Size The size of the data to transmit
 * @param Timeout (ignored)
 * @return HAL_StatusTypeDef The status of the peripheral 
 */
uint32_t emulator_lora_spi_transmit
    (
    void *hspi, 
    uint8_t *pData, 
    uint16_t Size, 
    uint32_t Timeout
    )
{
(void)hspi;
(void)Timeout;

if (pData == NULL || Size == 0U) {
    return HAL_OK;
}

if (!emulator_lora_state.initialized) {
    emulator_lora_reset_state();
    emulator_lora_state.initialized = true;
}

emulator_lora_start_thread_if_needed();

if (emulator_lora_state.spi_state == EMULATOR_LORA_SPI_READ_REG) {
    const uint8_t reg = emulator_lora_state.pending_reg;
    if (reg == LORA_REG_FIFO_RW && emulator_lora_state.fifo_len > 0U) {
        size_t bytes_to_read = (size_t)Size;
        if (bytes_to_read > emulator_lora_state.fifo_len) {
            bytes_to_read = emulator_lora_state.fifo_len;
        }

        memcpy(pData, emulator_lora_state.fifo, bytes_to_read);
        if (bytes_to_read < (size_t)Size) {
            memset(&pData[bytes_to_read], 0x00U, (size_t)Size - bytes_to_read);
        }

        if (bytes_to_read > 0U) {
            memmove(emulator_lora_state.fifo,
                    &emulator_lora_state.fifo[bytes_to_read],
                    emulator_lora_state.fifo_len - (uint8_t)bytes_to_read);
            emulator_lora_state.fifo_len = (uint8_t)(emulator_lora_state.fifo_len - bytes_to_read);
        }

        emulator_lora_state.registers[LORA_REG_FIFO_SPI_POINTER] = (uint8_t)(emulator_lora_state.fifo_spi_pointer + (uint8_t)bytes_to_read);
        emulator_lora_state.registers[LORA_REG_FIFO_RX_NUM_BYTES] = emulator_lora_state.fifo_len;
    }
    else {
        for (uint16_t idx = 0; idx < Size; ++idx) {
            pData[idx] = emulator_lora_state.registers[reg];
        }
    }

    emulator_lora_state.spi_state = EMULATOR_LORA_SPI_IDLE;
    return HAL_OK;
}

if (Size == 1U && (pData[0] & 0x80U) == 0U) {
    emulator_lora_state.pending_reg = (uint8_t)(pData[0] & 0x7FU);
    emulator_lora_state.pending_reg_write = false;
    emulator_lora_state.spi_state = EMULATOR_LORA_SPI_READ_REG;
    return HAL_OK;
}

if (Size == 1U && (pData[0] & 0x80U) != 0U) {
    emulator_lora_state.pending_reg = (uint8_t)(pData[0] & 0x7FU);
    emulator_lora_state.pending_reg_write = true;
    emulator_lora_state.spi_state = EMULATOR_LORA_SPI_WRITE_REG;
    return HAL_OK;
}

if (emulator_lora_state.pending_reg_write) {
    const uint8_t reg = emulator_lora_state.pending_reg;
    emulator_lora_state.pending_reg_write = false;

    if (reg == LORA_REG_FIFO_RW) {
        emulator_lora_handle_payload(&pData[1], Size - 1U);
    }
    else if (Size >= 2U) {
        emulator_lora_set_register(reg, pData[1]);
    }
    else {
        emulator_lora_set_register(reg, pData[0]);
    }

    emulator_lora_state.spi_state = EMULATOR_LORA_SPI_IDLE;
    return HAL_OK;
}

if ((pData[0] & 0x80U) != 0U) {
    const uint8_t reg = (uint8_t)(pData[0] & 0x7FU);
    if (Size == 2U) {
        emulator_lora_set_register(reg, pData[1]);
    }
    else if (Size > 1U) {
        emulator_lora_handle_payload(&pData[1], Size - 1U);
    }
}

emulator_lora_state.spi_state = EMULATOR_LORA_SPI_IDLE;
return HAL_OK;

} /* emulator_lora_spi_transmit */


/**
 * @brief Async mock for lora_transmit_async()'s DMA payload burst.
 * @note  The address byte (FIFO_RW | 0x80) was already sent through the
 *        blocking path above, which armed pending_reg/pending_reg_write (pure payload)
 *        
 *        Completion is delivered via lora_process_async_cb() to match real
 *        HAL_SPI_Transmit_DMA()/HAL_SPI_TxCpltCallback() semantics.
 *
 * @param hspi SPI handle (ignored)
 * @param pData Pointer to the payload to transmit
 * @param Size The size of the payload
 * @return uint32_t The status of the peripheral
 */
uint32_t emulator_lora_spi_transmit_dma
    (
    void *hspi,
    const uint8_t *pData,
    uint16_t Size
    )
{
(void)hspi;

if (pData == NULL || Size == 0U) {
    return HAL_OK;
}

if (!emulator_lora_state.initialized) {
    emulator_lora_reset_state();
    emulator_lora_state.initialized = true;
}

emulator_lora_start_thread_if_needed();

emulator_lora_handle_payload(pData, Size);
emulator_lora_state.pending_reg_write = false;
emulator_lora_state.spi_state = EMULATOR_LORA_SPI_IDLE;
emulator_lora_state.dma_op_is_tx = true;
emulator_lora_schedule_completion(EMULATOR_LORA_EVENT_DMA_CPLT);

return HAL_OK;

} /* emulator_lora_spi_transmit_dma */


/**
 * @brief Async mock for lora_request_receive_async()'s DMA payload burst.
 * @note  The address byte was sent through the blocking path above,
 *        which armed pending_reg/spi_state==READ_REG.
 *        Should mirror continuation's FIFO read, async. pTxData is the dummy clocking
 *        bytes and carries no meaning on this side.
 *
 * @param hspi SPI handle (ignored)
 * @param pTxData Dummy TX bytes (ignored)
 * @param pRxData Destination buffer for the received payload
 * @param Size The size of the payload to receive
 * @return uint32_t The status of the peripheral
 */
uint32_t emulator_lora_spi_transmit_receive_dma
    (
    void *hspi,
    const uint8_t *pTxData,
    uint8_t *pRxData,
    uint16_t Size
    )
{
(void)hspi;
(void)pTxData;

if (pRxData == NULL || Size == 0U) {
    return HAL_OK;
}

if (!emulator_lora_state.initialized) {
    emulator_lora_reset_state();
    emulator_lora_state.initialized = true;
}

emulator_lora_start_thread_if_needed();

size_t bytes_to_read = (size_t)Size;
if (bytes_to_read > emulator_lora_state.fifo_len) {
    bytes_to_read = emulator_lora_state.fifo_len;
}

memcpy(pRxData, emulator_lora_state.fifo, bytes_to_read);
if (bytes_to_read < (size_t)Size) {
    memset(&pRxData[bytes_to_read], 0x00U, (size_t)Size - bytes_to_read);
}

if (bytes_to_read > 0U) {
    memmove(emulator_lora_state.fifo,
            &emulator_lora_state.fifo[bytes_to_read],
            emulator_lora_state.fifo_len - (uint8_t)bytes_to_read);
    emulator_lora_state.fifo_len = (uint8_t)(emulator_lora_state.fifo_len - bytes_to_read);
}

emulator_lora_state.registers[LORA_REG_FIFO_SPI_POINTER] = (uint8_t)(emulator_lora_state.fifo_spi_pointer + (uint8_t)bytes_to_read);
emulator_lora_state.registers[LORA_REG_FIFO_RX_NUM_BYTES] = emulator_lora_state.fifo_len;

emulator_lora_state.spi_state = EMULATOR_LORA_SPI_IDLE;
emulator_lora_state.dma_op_is_tx = false;
emulator_lora_schedule_completion(EMULATOR_LORA_EVENT_DMA_CPLT);

return HAL_OK;

} /* emulator_lora_spi_transmit_receive_dma */


/**
 * @brief Schedule an event that would triggern an ISR.
 * 
 * @param event The event to schedule.
 */
static void emulator_lora_schedule_completion
    (
    EMULATOR_LORA_EVENT event
    )
{
pthread_mutex_lock(&emulator_lora_it_mutex);
emulator_lora_state.pending_event = event;
pthread_cond_signal(&emulator_lora_it_cond);
pthread_mutex_unlock(&emulator_lora_it_mutex);

} /* emulator_lora_schedule_completion */


/**
 * @brief Start the LoRa listener thread
 */
static void emulator_lora_start_thread_if_needed(void)
{
if (emulator_lora_it_thread_started) {
    return;
}

if (pthread_create(&emulator_lora_it_thread, NULL, emulator_lora_it_listener, NULL) != 0) {
    emulator_debug_logf("Failed to start LoRa completion thread", EMULATOR_SUBSYSTEM_LORA);
    return;
}

emulator_lora_it_thread_started = true;

} /* emulator_lora_start_thread_if_needed */

/**
 * @brief Listen for events that would trigger an ISR.
 * 
 * @param arg (ignored)
 * @return void* (ignored)
 */
static void* emulator_lora_it_listener
    (
    void *arg
    )
{
(void)arg;

for (;;) {
    pthread_mutex_lock(&emulator_lora_it_mutex);
    while (emulator_lora_state.pending_event == EMULATOR_LORA_EVENT_NONE) {
        pthread_cond_wait(&emulator_lora_it_cond, &emulator_lora_it_mutex);
    }

    const EMULATOR_LORA_EVENT event = emulator_lora_state.pending_event;
    emulator_lora_state.pending_event = EMULATOR_LORA_EVENT_NONE;
    pthread_mutex_unlock(&emulator_lora_it_mutex);

    if (event == EMULATOR_LORA_EVENT_DMA_CPLT) {
        const bool was_tx = emulator_lora_state.dma_op_is_tx;

        /* Simulated SPI DMA burst transfer time. */
        usleep(10000U);
        lora_process_async_cb();

        if (was_tx) {
            /* The radio still needs real airtime before DIO0 signals TxDone. */  
            usleep(20000U);
            lora_process_dio0_cb();
        }
    }
}

return NULL;

} /* emulator_lora_it_listener */

/*------------------------------------------------------------------------------
 Procedures                                                      
------------------------------------------------------------------------------*/
/**
 * @brief Reset state data in the RFM95 mock driver
 */
static void emulator_lora_reset_state
    (
    void
    )
{
emulator_debug_logf( "Resetting State", EMULATOR_SUBSYSTEM_LORA);
memset(&emulator_lora_state, 0, sizeof(emulator_lora_state));

emulator_lora_state.registers[LORA_REG_ID_VERSION] = LORA_ID_VERSION_VAL;
emulator_lora_state.registers[LORA_REG_OPERATION_MODE] = LORA_SLEEP_MODE;
emulator_lora_state.registers[LORA_REG_FIFO_TX_BASE_ADDR] = 0x00U;
emulator_lora_state.registers[LORA_REG_FIFO_SPI_POINTER] = 0x00U;
emulator_lora_state.registers[LORA_REG_FIFO_RX_NUM_BYTES] = 0x00U;
emulator_lora_state.registers[LORA_REG_PA_CONFIG] = 0x7FU;
emulator_lora_state.fifo_tx_base_addr = 0x00U;
emulator_lora_state.fifo_spi_pointer = 0x00U;

} /* emulator_lora_reset_state */


/**
 * @brief Set the value of an RFM95 register.
 * 
 * @param reg The register address
 * @param value The value to store
 */
static void emulator_lora_set_register
    (
    uint8_t reg, 
    uint8_t value
    )
{
emulator_lora_state.registers[reg] = value;

if (reg == LORA_REG_OPERATION_MODE) {
    const uint8_t mode = (uint8_t)(value & 0x07U);
    emulator_lora_state.tx_pending = (mode == LORA_TRANSMIT_MODE);

    if (mode == LORA_TRANSMIT_MODE) {
        const uint8_t lo_ra_bit = (uint8_t)(value & 0x80U);
        emulator_lora_state.registers[reg] = (uint8_t)(lo_ra_bit | LORA_STANDBY_MODE);
        emulator_lora_execute_tx();
        emulator_lora_state.tx_pending = false;
    }
}
else if (reg == LORA_REG_FIFO_TX_BASE_ADDR) {
    emulator_lora_state.fifo_tx_base_addr = value;
    emulator_lora_state.fifo_spi_pointer = value;
    emulator_lora_state.registers[LORA_REG_FIFO_SPI_POINTER] = value;
}
else if (reg == LORA_REG_FIFO_SPI_POINTER) {
    emulator_lora_state.fifo_spi_pointer = value;
}

} /* emulator_lora_set_register */


/**
 * @brief Store data in the LoRa FIFO.
 * 
 * @param data The message to store
 * @param len The length of the message
 */
static void emulator_lora_store_fifo_bytes(const uint8_t *data, size_t len)
{
if (data == NULL || len == 0U) {
    return;
}

size_t bytes_to_store = len;
size_t available_space = sizeof(emulator_lora_state.fifo) - emulator_lora_state.fifo_len;
if (bytes_to_store > available_space) {
    bytes_to_store = available_space;
}

memcpy(&emulator_lora_state.fifo[emulator_lora_state.fifo_len], data, bytes_to_store);
emulator_lora_state.fifo_len = (uint8_t)(emulator_lora_state.fifo_len + bytes_to_store);
emulator_lora_state.registers[LORA_REG_FIFO_SPI_POINTER] = (uint8_t)(emulator_lora_state.fifo_tx_base_addr + emulator_lora_state.fifo_len);
emulator_lora_state.registers[LORA_REG_FIFO_RX_NUM_BYTES] = emulator_lora_state.fifo_len;

} /* emulator_lora_store_fifo_bytes */


/**
 * @brief Store a message in the LoRa FIFO.
 * 
 * @param data The message to store.
 * @param len The length of the message.
 */
static void emulator_lora_handle_payload(const uint8_t *data, size_t len)
{
if (data == NULL || len == 0U) {
    return;
}

if (len > sizeof(emulator_lora_state.last_payload)) {
    len = sizeof(emulator_lora_state.last_payload);
}

memcpy(emulator_lora_state.last_payload, data, len);
emulator_lora_state.last_payload_len = len;
emulator_lora_store_fifo_bytes(data, len);

} /* emulator_lora_handle_payload */


/**
 * @brief Execute a LoRa transmission
 */
static void emulator_lora_execute_tx
    (
    void
    )
{
if (emulator_lora_state.fifo_len == 0U && emulator_lora_state.last_payload_len == 0U) {
    emulator_lora_state.irq_flags = 0x08U;
    emulator_lora_state.registers[LORA_REG_IRQ_FLAGS] = emulator_lora_state.irq_flags;
    return;
}

if (emulator_lora_state.last_payload_len == 0U && emulator_lora_state.fifo_len > 0U) {
    memcpy(emulator_lora_state.last_payload, emulator_lora_state.fifo, emulator_lora_state.fifo_len);
    emulator_lora_state.last_payload_len = emulator_lora_state.fifo_len;
}

emulator_lora_state.last_tx_time_ms = (uint32_t)emulator_lora_state.last_payload_len;
emulator_lora_state.irq_flags = 0x08U;
emulator_lora_state.registers[LORA_REG_IRQ_FLAGS] = emulator_lora_state.irq_flags;
lora_ota_transmit((void *)emulator_lora_state.last_payload, emulator_lora_state.last_payload_len, emulator_lora_state.last_tx_time_ms);

memset(emulator_lora_state.fifo, 0x00U, sizeof(emulator_lora_state.fifo));
emulator_lora_state.fifo_len = 0U;
emulator_lora_state.registers[LORA_REG_FIFO_RX_NUM_BYTES] = 0x00U;
emulator_lora_state.registers[LORA_REG_FIFO_SPI_POINTER] = emulator_lora_state.fifo_tx_base_addr;

} /* emulator_lora_execute_tx */


/**
 * @brief Mocked version of RFM95 transmission; sends bytes to the mocked ground station.
 *
 * @param data The data to transmit
 * @param len The length of the message
 * @param time_to_tx_ms The time to simulate over the air (delays the GS from reading the message)
 */
static void lora_ota_transmit
    (
    void *data, 
    size_t len, 
    uint32_t time_to_tx_ms
    )
{
(void)time_to_tx_ms;

if (data == NULL || len == 0U) {
    return;
}
emulator_lora_state.last_tx_time_ms = HAL_GetTick();
emulator_gs_update_buf
    (
    data, 
    len, 
    time_to_tx_ms
    );

} /* lora_ota_transmit */
