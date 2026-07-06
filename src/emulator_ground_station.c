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
 Static Prototypes                                                     
------------------------------------------------------------------------------*/
static USB_STATUS serial_read
    (
    void*    rx_data_ptr , /* Buffer to export data to        */
	size_t   rx_data_size  /* Size of the data to be received */
    );

static void serial_write
    (
    const uint8_t* msg,
    size_t len
    );

/*------------------------------------------------------------------------------
 Procedures                                                     
------------------------------------------------------------------------------*/

/**
* @brief Set up the serial connection to SDEC on the GS.                                  
*/
bool emulator_prompt_and_open_serial_port_gs
    (
    void
    )
{
/* Prompt for serial port */
char port_buf[12];
emulator_log("Please enter your serial port in the format /dev/ttyXX or in the format COMX.", EMULATOR_SUBSYSTEM_GS);
#if defined( _WIN32 ) || defined( __CYGWIN__ )
#endif
printf("Input: \n");

if(fgets(port_buf, sizeof(port_buf), stdin) == NULL){
    emulator_log("Invalid port input.", EMULATOR_SUBSYSTEM_GS);
    return false;
}

port_buf[strcspn(port_buf, "\n")] = 0;

char com_buf[4];
int com_port_num;

strncpy(com_buf, port_buf, 3);

if(strncmp(com_buf, "COM", 3) == 0){
    sscanf(port_buf+3, "%d", &com_port_num);
    com_port_num--;
    uint8_t last_two_digits = com_port_num % 100; /* least significant two digits */

    /* we know this is safe, but we need to ignore the warning */
    // ETS: THIS IS GROSS. Do not do this.
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wformat-truncation"
    snprintf(port_buf, 12, "/dev/ttyS%u", last_two_digits);
    #pragma GCC diagnostic pop
}

serial_port_gs = open(port_buf, O_RDWR | O_NOCTTY | O_NDELAY); // Open the port

if (serial_port_gs < 0) {
    emulator_log("Error opening serial port.", EMULATOR_SUBSYSTEM_GS);
    return false;
}

struct termios tty;

// Read in existing settings, handle errors
if(tcgetattr(serial_port_gs, &tty) != 0) {
    emulator_log("tcgetattr failed.", EMULATOR_SUBSYSTEM_GS);
    return false;
}

// Configure port settings (baud rate, parity, etc.)
cfsetospeed(&tty, B921600); // Set output baud rate to 921600
cfsetispeed(&tty, B921600); // Set input baud rate to 921600

tty.c_cflag &= ~PARENB;        // No parity
tty.c_cflag &= ~CSTOPB;        // One stop bit
tty.c_cflag &= ~CSIZE;         // Clear size bits
tty.c_cflag |= CS8;            // 8 data bits
tty.c_cflag &= ~CRTSCTS;       // No hardware flow control
tty.c_cflag |= CREAD | CLOCAL; // Enable reading and ignore modem control lines

tty.c_lflag &= ~ICANON; // Disable canonical mode (line-by-line input)
tty.c_lflag &= ~ECHO;   // Disable echo
tty.c_lflag &= ~ECHOE;  // Disable erasure
tty.c_lflag &= ~ECHONL; // Disable new-line echo
tty.c_lflag &= ~ISIG;   // Disable interpretation of signal characters

tty.c_iflag &= ~(IXON | IXOFF | IXANY); // Disable software flow control
tty.c_iflag &= ~(IGNBRK|BRKINT|PARMRK|ISTRIP|INLCR|IGNCR|ICRNL); // Disable special handling of bytes

tty.c_oflag &= ~OPOST; // Prevent special interpretation of output bytes
tty.c_oflag &= ~ONLCR; // Prevent conversion of newline to carriage return/line feed

tty.c_cc[VTIME] = 1;   // Wait for up to 0.1 seconds (100 ms; 1 decisecond)
tty.c_cc[VMIN] = 0;    // Minimum number of characters to read

// Save TTY settings, handle errors
if (tcsetattr(serial_port_gs, TCSANOW, &tty) != 0) {
    emulator_log("tcsetattr failed.", EMULATOR_SUBSYSTEM_GS);
    return false;
}

if (fcntl(serial_port_gs, F_GETFD) == -1) {
    emulator_log("FD became invalid.", EMULATOR_SUBSYSTEM_GS);
    return false;
}
else {
    emulator_log("FD remails valid at end of init.", EMULATOR_SUBSYSTEM_GS);
}

return true;

} /* emulator_prompt_and_open_serial_port_gs */


/**
* @brief Write to the virtual serial port.                                      
*/
static void serial_write
    (
    const uint8_t* msg,
    size_t len
    )
{
if ( serial_port_gs < 0 )
    {
    return;
    }

write( serial_port_gs, msg, len );

} /* serial_write */


/**
 * @brief Read from the VCP
 * 
 * @param rx_data_ptr The buffer to read to
 * @param rx_data_size The size of the buffer
 *
 * @return The status of the read
 */
static USB_STATUS serial_read
    (
    void*    rx_data_ptr , /* Buffer to export data to        */
	size_t   rx_data_size  /* Size of the data to be received */
    )
{

// Verify fd is still valid before reading
if (fcntl(serial_port_gs, F_GETFD) == -1) {
    emulator_log("Read: File descriptor became invalid.", EMULATOR_SUBSYSTEM_GS);
    return USB_FAIL;
}

/* Blocking read */
struct timeval tv;
tv.tv_sec  = 0;
tv.tv_usec = 10000; // 10 ms

fd_set rfds;
FD_ZERO(&rfds);
FD_SET(serial_port_gs, &rfds);

int ret = select(serial_port_gs + 1, &rfds, NULL, NULL, &tv);
if (ret < 0) {
    emulator_log("Select failed.", EMULATOR_SUBSYSTEM_GS);
    return USB_FAIL;
} else if (ret == 0) {
    return USB_TIMEOUT;
}

memset( rx_data_ptr, 0, rx_data_size );
int n = read( serial_port_gs, rx_data_ptr, rx_data_size );
    
    if (n < 0) {
        emulator_log("Read failed.", EMULATOR_SUBSYSTEM_GS);
        return USB_FAIL;
    } else if (n == 0) {
        return USB_TIMEOUT;
    } else {
        return USB_OK;
    }

} /* serial_read */

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
    rx_status = serial_read( rx_buf, 1 );
    
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
            serial_write( tx_buf, 1 );

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
            serial_write( tx_buf, 2 );
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
                    serial_write( tx_buf, len );
                    state_buf[i].timestamp_available = 0xFFFFFFFF;
                    tx_cplt = true;
                    //emulator_debug_logf( "Used an existing message: %d bytes", EMULATOR_SUBSYSTEM_GS, len );
                    }
                }
            pthread_mutex_unlock(&state_mutex);
            if( !tx_cplt )
                {
                memset( tx_buf, 0, 256 );
                serial_write( tx_buf, TELEMETRY_MESSAGE_SIZE );
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