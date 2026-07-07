/**
* @file emulator_ground_station.c
*
* Mocks the functionality of the ground station.
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
#include "math_sdr.h"
#include "telemetry.h"

/* POSIX */
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <pthread.h>

/*------------------------------------------------------------------------------
 Constants                                                       
------------------------------------------------------------------------------*/
#define GS_HW_CODE 0x10
#define GS_FW_CODE 0x11

/*------------------------------------------------------------------------------
 Globals                                                       
------------------------------------------------------------------------------*/
int serial_port_gs = -1;

typedef struct TELEM_STATE {
    uint32_t timestamp_available;
    uint8_t data[128];
    size_t len;
} TELEM_STATE;

static TELEM_STATE state_buf[8] = { 
    [0 ... 7] = { .timestamp_available = 0xFFFFFFFF } /* GNU ONLY */
};
static uint8_t     buf_ptr = 0;

static pthread_mutex_t state_mutex = PTHREAD_MUTEX_INITIALIZER;

/*------------------------------------------------------------------------------
 Procedures                                                     
------------------------------------------------------------------------------*/

/**
 * @brief Add a new message to the ground station buffer
 * 
 * @param data The message to add
 * @param len The length of the message
 * @param time_to_tx_ms The amount of time to wait before using the message
 */
void emulator_gs_update_buf
    (
    void *data, 
    size_t len, 
    uint32_t time_to_tx_ms
    )
{
if( len > 128 && len != 1 ) /* catch bad condition */
    {
    return;
    }

pthread_mutex_lock(&state_mutex);
state_buf[ buf_ptr ].timestamp_available = HAL_GetTick() + time_to_tx_ms;
memcpy( &(state_buf[ buf_ptr ].data), data, len );
state_buf[ buf_ptr ].len = len;

buf_ptr++;

if( buf_ptr > 7 )
    {
    buf_ptr = 0;
    }
pthread_mutex_unlock(&state_mutex);

} /* emulator_gs_update_buf */


/**
  * @brief The worker thread for the ground station mock interface.
  */
void* emulator_gs_terminal_loop
    (
    void* arg
    )
{
/*---------------------------------------------------------------------
 Local Variables                                                             
---------------------------------------------------------------------*/
(void)arg;
uint8_t tx_buf[ 256 ];
uint8_t rx_buf[ 256 ];
USB_STATUS rx_status = USB_OK;

while ( 1 )
    {
    memset( rx_buf, 0, 256 );
    memset( tx_buf, 0, 256 );
    rx_status = emulator_serial_read( GS_SERIAL_PORT, rx_buf, 1 );
    
    if( rx_status == USB_FAIL )
        {
        break;
        }
    else if( rx_status == USB_TIMEOUT )
        {
        continue;
        }
    
    /*-----------------------------------------------------------------
    Terminal Command Handler                                                               
    -----------------------------------------------------------------*/
    switch ( rx_buf[0] )
        {
        /*-------------------------------------------------------------
            PING_OP	
        -------------------------------------------------------------*/
        case PING_OP:
            {
            /* Send board identifying code    */
            tx_buf[ 0 ] = GS_HW_CODE;
            emulator_serial_write( GS_SERIAL_PORT, tx_buf, 1 );

            break;
            } /* PING_OP */
        /*-------------------------------------------------------------
            CONNECT_OP	
        -------------------------------------------------------------*/
        case CONNECT_OP:
            {
            emulator_debug_logf( "Dashboard connection received", EMULATOR_SUBSYSTEM_GS );
            /* Send board identifying code    */
            tx_buf[ 0 ] = GS_HW_CODE;
            tx_buf[ 1 ] = GS_FW_CODE;

            /* Send firmware identifying code */
            emulator_serial_write( GS_SERIAL_PORT, tx_buf, 2 );
            break;
            } /* CONNECT_OP */
        /*--------------------------------------------------------------
            DASHBOARD Command	
        --------------------------------------------------------------*/
        case DASHBOARD_OP:
            {
            //emulator_debug_logf( "Dashboard OP received", EMULATOR_SUBSYSTEM_GS );
            bool tx_cplt = false;
            pthread_mutex_lock(&state_mutex);
            for( int i = 0; i < 8; i++ )
                {
                if( state_buf[i].timestamp_available != 0xFFFFFFFF && state_buf[i].timestamp_available <= HAL_GetTick() )
                    {
                    uint8_t len = state_buf[i].len;
                    memcpy( tx_buf, &(state_buf[i].data), len );
                    emulator_serial_write( GS_SERIAL_PORT, tx_buf, len );
                    state_buf[i].timestamp_available = 0xFFFFFFFF;
                    tx_cplt = true;
                    //emulator_debug_logf( "Used an existing message: %d bytes", EMULATOR_SUBSYSTEM_GS, len );
                    }
                }
            pthread_mutex_unlock(&state_mutex);
            if( !tx_cplt )
                {
                memset( tx_buf, 0, 256 );
                emulator_serial_write( GS_SERIAL_PORT, tx_buf, TELEMETRY_MESSAGE_SIZE );
                //emulator_debug_logf( "Used a blank message: %d bytes", EMULATOR_SUBSYSTEM_GS, TELEMETRY_MESSAGE_SIZE );
                }
            }
            break;
        /*-------------------------------------------------------------
            Unrecognized command code  
        -------------------------------------------------------------*/
        default:
            {
            // TODO: Give warning ( via error_fail_safe() )
            //error_fail_fast();
            break;
            }

        } /* switch( usb_rx_data ) */
    } /* while(1) */

emulator_debug_logf( "Ground station thread is exiting.", EMULATOR_SUBSYSTEM_GS );

return NULL;

} /* emulator_gs_terminal_loop */