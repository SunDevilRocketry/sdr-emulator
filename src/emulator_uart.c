/**
* @file emulator_uart.c
*
* Mocks the functionality of UART peripherals on the FC.
*                                                                             
* @copyright
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
/* Standard */
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>

/* Project */
#include "emulator.h"
#include "stm32h7xx_hal.h"
#include "sdr_pin_defines_A0002.h"
#include "usb.h"
#include "gps.h"
#include "math_sdr.h"

/* POSIX */
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <pthread.h>

#define GPS_SIM_DELAY 200 /* 200 ms - approx 10 frames */

/*------------------------------------------------------------------------------
 Globals                                                       
------------------------------------------------------------------------------*/
extern uint8_t            gps_mesg_byte;
extern uint8_t            rx_buffer[GPSBUFSIZE];
extern GPS_DATA           gps_data;

volatile bool gps_data_it_flag = false;

/* Shared synchronization objects */
static pthread_mutex_t uart_it_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  uart_it_cond  = PTHREAD_COND_INITIALIZER;

/* Mock GPS messages */
static const char* gps_msgs[] = 
{
"$GPRMC,134730.361,A,5540.3220,N,01231.2858,E,1.06,86.57,041112,,,A*55",
"$GPRMC,134731.361,A,5540.3252,N,01231.2946,E,1.42,93.80,041112,,,A*51",
"$GPVTG,93.80,T,,M,1.42,N,2.6,K,A*3C",
"$GPGGA,134732.000,5540.3244,N,01231.2941,E,1,10,0.8,31.7,M,41.5,M,,0000*6A",
"$GPRMC,134732.000,A,5540.3244,N,01231.2941,E,1.75,90.16,041112,,,A*5E",
"$GPVTG,90.16,T,,M,1.75,N,3.2,K,A*31",
"$GPGGA,134733.000,5540.3231,N,01231.2938,E,1,10,0.8,24.9,M,41.5,M,,0000*6D",
"$GPRMC,134733.000,A,5540.3231,N,01231.2938,E,1.83,113.00,041112,,,A*67",
"$GPRMC,134734.000,A,5540.3233,N,01231.2941,E,1.23,107.22,041112,,,A*63",
"$GPGSA,A,3,03,22,06,19,11,14,32,01,28,18,,,1.8,0.8,1.6*3F",
"$GPGLL,3953.88008971,N,10506.75318910,W,034138.00,A,D*7A",
"$GPGLL,5109.0262317,N,11401.8407304,W,202725.00,A,D*79",
};

/*------------------------------------------------------------------------------
 Static Prototypes                                                     
------------------------------------------------------------------------------*/

static void gps_read_handler_IT
    (
    int message_num
    );

/*------------------------------------------------------------------------------
 HAL interfaces                                                       
------------------------------------------------------------------------------*/

HAL_StatusTypeDef HAL_UART_Transmit(UART_HandleTypeDef *huart, const uint8_t *pData, uint16_t Size, uint32_t Timeout) {
    if ( huart == &(USB_HUART) )
        {
        emulator_serial_write( FC_SERIAL_PORT, pData, (size_t)Size );
        }
    return HAL_OK;
}

HAL_StatusTypeDef HAL_UART_Transmit_IT(UART_HandleTypeDef *huart, const uint8_t *pData, uint16_t Size) {
    if ( huart == &(USB_HUART) )
        {
        emulator_serial_write( FC_SERIAL_PORT, pData, (size_t)Size );
        }
    return HAL_OK;
}

HAL_StatusTypeDef HAL_UART_Receive(UART_HandleTypeDef *huart, uint8_t *pData, uint16_t Size, uint32_t Timeout) {
    if ( huart == &(USB_HUART) )
        {
        emulator_serial_read( FC_SERIAL_PORT, pData, (size_t)Size );
        }
    return HAL_OK;
}

HAL_StatusTypeDef HAL_UART_Receive_IT(UART_HandleTypeDef *huart, uint8_t *pData, uint16_t Size) {
    if ( huart == &(GPS_HUART) )
        {
        pthread_mutex_lock(&uart_it_mutex);
        gps_data_it_flag = true;
        pthread_cond_signal(&uart_it_cond);
        pthread_mutex_unlock(&uart_it_mutex);
        }
    return HAL_OK;
}

/*------------------------------------------------------------------------------
 Procedures                                                     
------------------------------------------------------------------------------*/

/**
* Listen for and fulfill UART GPS IT I/O.                                
*/
void* emulator_gps_it_listener
    (
    void* arg
    )
{
bool recieve_gps = true;
int gps_msg_idx = 0;
while (recieve_gps) 
    {
    /* Wait until someone signals an IRQ */
    pthread_mutex_lock(&uart_it_mutex);

    /* Wait for work - handle spurious wakeups */
    while (!gps_data_it_flag) {
        pthread_cond_wait(&uart_it_cond, &uart_it_mutex);
    }

    pthread_mutex_unlock(&uart_it_mutex);

    /* Simulate real-time 50 ms I/O delay */
    struct timespec req;
    req.tv_sec = 0; /* seconds */
    req.tv_nsec = GPS_SIM_DELAY * 1000000L; /* milliseconds to nanoseconds */
    nanosleep(&req, NULL);

    if ( emulator_flags_check_bits(IRQ_ENABLED_FLAG_BIT) ) 
        {
        pthread_mutex_lock(&uart_it_mutex); /* lock to safely access shared flags */
        gps_data_it_flag = false;
        pthread_mutex_unlock(&uart_it_mutex);

        gps_read_handler_IT(gps_msg_idx);
        }

    if (++gps_msg_idx >= array_size( gps_msgs ) )
        {
        gps_msg_idx = 0;
        }
    }

return 0;

} /* emulator_gps_it_listener */


/**
* Interrupt the main thread with a new GPS message.                      
*/
static void gps_read_handler_IT
    (
    int message_num
    )
{
// memset( gps_data_ptr, 0, gps_data_size );
if (message_num >= array_size( gps_msgs ) )
    {
    emulator_log("Index out of range.", EMULATOR_SUBSYSTEM_GPS);
    return;
    }
memcpy( rx_buffer, gps_msgs[message_num], strlen( gps_msgs[message_num] ) );

/* Pasted in from UART4_IRQHandler */
if(gps_mesg_validate((char*) rx_buffer))
    GPS_parse(&gps_data, (char*) rx_buffer);

memset(rx_buffer, 0, sizeof(rx_buffer));

gps_receive_IT(&gps_mesg_byte, 1);

} /* gps_read_handler_IT */
