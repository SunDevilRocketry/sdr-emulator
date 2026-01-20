/*******************************************************************************
*
* FILE: 
* 		emulator_i2c.c
*
* DESCRIPTION: 
* 		Mocks the functionality of I2C peripherals on the FC.
*                                                                             
* COPYRIGHT:                                                                  
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
*******************************************************************************/

/*------------------------------------------------------------------------------
 Includes                                                         
------------------------------------------------------------------------------*/
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include "emulator.h"
#include "stm32h7xx_hal.h"
#include "baro.h"
#include "imu.h"
#include "sdr_pin_defines_A0002.h"

/*------------------------------------------------------------------------------
 Globals                                                       
------------------------------------------------------------------------------*/
extern volatile bool irq_enabled;

volatile bool imu_data_it_flag = false;
volatile bool baro_data_it_flag = false;
volatile bool mag_data_it_flag = false;

static uint8_t* imu_data_ptr;
static uint8_t* baro_data_ptr;
static uint8_t* mag_data_ptr;
static uint16_t imu_data_size = 0;
static uint16_t baro_data_size = 0;
static uint16_t mag_data_size = 0;

/*------------------------------------------------------------------------------
 Procedure prototypes                                                       
------------------------------------------------------------------------------*/
static HAL_StatusTypeDef baro_read_handler(I2C_HandleTypeDef *hi2c, uint16_t DevAddress, uint16_t MemAddress,
                                    uint16_t MemAddSize, uint8_t *pData, uint16_t Size, uint32_t Timeout);
static HAL_StatusTypeDef imu_read_handler(I2C_HandleTypeDef *hi2c, uint16_t DevAddress, uint16_t MemAddress,
                                    uint16_t MemAddSize, uint8_t *pData, uint16_t Size, uint32_t Timeout);
static HAL_StatusTypeDef mag_read_handler(I2C_HandleTypeDef *hi2c, uint16_t DevAddress, uint16_t MemAddress,
                                    uint16_t MemAddSize, uint8_t *pData, uint16_t Size, uint32_t Timeout);
static void baro_read_handler_IT();
static void imu_read_handler_IT();
static void mag_read_handler_IT();

/*------------------------------------------------------------------------------
 HAL interfaces                                                       
------------------------------------------------------------------------------*/

HAL_StatusTypeDef HAL_I2C_Mem_Write(I2C_HandleTypeDef *hi2c, uint16_t DevAddress, uint16_t MemAddress,
                                    uint16_t MemAddSize, uint8_t *pData, uint16_t Size, uint32_t Timeout) {
    return HAL_OK;
}

HAL_StatusTypeDef HAL_I2C_Mem_Read(I2C_HandleTypeDef *hi2c, uint16_t DevAddress, uint16_t MemAddress,
                                    uint16_t MemAddSize, uint8_t *pData, uint16_t Size, uint32_t Timeout) {
if( hi2c == &( BARO_I2C ) )
    {
    return baro_read_handler( hi2c, DevAddress, MemAddress, MemAddSize, pData, Size, Timeout);
    }
else if( ( hi2c == &( IMU_I2C ) ) 
   && ( DevAddress == IMU_ADDR ) )
    {
    return imu_read_handler( hi2c, DevAddress, MemAddress, MemAddSize, pData, Size, Timeout);
    }
else if( ( hi2c == &( IMU_I2C ) ) 
   && ( DevAddress == IMU_MAG_ADDR ) )
    {
    return mag_read_handler( hi2c, DevAddress, MemAddress, MemAddSize, pData, Size, Timeout);
    }

    return HAL_OK;
}

HAL_StatusTypeDef HAL_I2C_Mem_Read_IT(I2C_HandleTypeDef *hi2c, uint16_t DevAddress, uint16_t MemAddress,
                                    uint16_t MemAddSize, uint8_t *pData, uint16_t Size) {
if( hi2c == &( BARO_I2C ) )
    {
    baro_data_it_flag = true;
    baro_data_ptr = pData;
    baro_data_size = Size;
    return HAL_OK;
    }
else if( ( hi2c == &( IMU_I2C ) ) 
   && ( DevAddress == IMU_ADDR ) )
    {
    imu_data_it_flag = true;
    imu_data_ptr = pData;
    imu_data_size = Size;
    return HAL_OK;
    }
else if( ( hi2c == &( IMU_I2C ) ) 
   && ( DevAddress == IMU_MAG_ADDR ) )
    {
    mag_data_it_flag = true;
    mag_data_ptr = pData;
    mag_data_size = Size;
    return HAL_OK;
    }

return HAL_OK;
}

/*------------------------------------------------------------------------------
 Procedures                                                     
------------------------------------------------------------------------------*/

/*******************************************************************************
*                                                                              *
* PROCEDURE:                                                                   * 
* 		emulator_i2c_it_listener                                               *
*                                                                              *
* DESCRIPTION:                                                                 * 
*       Listen for and fulfill i2c IT I/O.                                     *
*                                                                              *
*******************************************************************************/
void* emulator_i2c_it_listener
    (
    void* arg
    )
{
bool listening = true;

while ( listening )
    {
    if ( irq_enabled )
        {
        /* Check each flag and call their handlers */
        if ( imu_data_it_flag )
            {
            imu_read_handler_IT();
            }
        if ( baro_data_it_flag )
            {
            baro_read_handler_IT();
            }
        if ( mag_data_it_flag )
            {
            mag_read_handler_IT();
            }
        }
    HAL_Delay(2);
    }

    return 0;
}

/* Blocking helpers -- used for init */

static HAL_StatusTypeDef baro_read_handler(I2C_HandleTypeDef *hi2c, uint16_t DevAddress, uint16_t MemAddress,
                                    uint16_t MemAddSize, uint8_t *pData, uint16_t Size, uint32_t Timeout)
{
    /* imperfect approximation, but delay a bit like the real FC */
    HAL_Delay( (uint8_t)(0.4 * Size) );

    if( MemAddress == BARO_REG_CHIP_ID )
        {
        *pData = BMP390_DEVICE_ID;
        return HAL_OK;
        }
    if( MemAddress == BARO_REG_ERR_REG )
        {
        *pData = 0x00; /* Clear the error register */
        return HAL_OK;
        }
    
    return HAL_OK;
}

static HAL_StatusTypeDef imu_read_handler(I2C_HandleTypeDef *hi2c, uint16_t DevAddress, uint16_t MemAddress,
                                    uint16_t MemAddSize, uint8_t *pData, uint16_t Size, uint32_t Timeout)
{
    /* imperfect approximation, but delay a bit like the real FC */
    HAL_Delay( (uint8_t)(0.4 * Size) );

    if( MemAddress == IMU_REG_CHIP_ID )
        {
        *pData = IMU_ID;
        return HAL_OK;
        }
    if( MemAddress == IMU_REG_INTERNAL_STATUS )
        {
        *pData = 0x01; /* First bit needs to be set */
        return HAL_OK;
        }
    
    return HAL_OK;
}


static HAL_StatusTypeDef mag_read_handler(I2C_HandleTypeDef *hi2c, uint16_t DevAddress, uint16_t MemAddress,
                                    uint16_t MemAddSize, uint8_t *pData, uint16_t Size, uint32_t Timeout)
{
    /* imperfect approximation, but delay a bit like the real FC */
    HAL_Delay( (uint8_t)(0.4 * Size) );

    if( MemAddress == MAG_REG_CHIP_ID )
        {
        *pData = MAG_ID;
        return HAL_OK;
        }
    if( MemAddress == IMU_REG_INTERNAL_STATUS )
        {
        *pData = 0x01; /* First bit needs to be set */
        return HAL_OK;
        }
    
    return HAL_OK;
}

/* Interrupt helpers -- used for data retrieval */
static void baro_read_handler_IT()
{
memset(baro_data_ptr, 0, baro_data_size);
baro_IT_handler();
}

static void imu_read_handler_IT()
{
memset(imu_data_ptr, 0, imu_data_size);
imu_it_handler(); 
}

static void mag_read_handler_IT()
{
memset(mag_data_ptr, 0, mag_data_size);
imu_it_handler();
}