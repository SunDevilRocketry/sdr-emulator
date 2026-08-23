/**
* @file emulator_serial.c
*
* Virtual serial port utility.
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
 Local Types                                                       
------------------------------------------------------------------------------*/
typedef struct _SERIAL_HANDLE
    {
    SERIAL_PORT port_id;
    int port_handle;
    } SERIAL_HANDLE;

/*------------------------------------------------------------------------------
 Statics                                                       
------------------------------------------------------------------------------*/
static SERIAL_HANDLE serial_ports[ SERIAL_PORT_COUNT ] = 
    { [0 ... SERIAL_PORT_COUNT - 1] = { .port_handle = -1 } }; /* initialize all file handles to 0 */

/*------------------------------------------------------------------------------
 Procedures                                                     
------------------------------------------------------------------------------*/

/**
* @brief Set up the serial port.
* @param port The port to open
*/
bool emulator_serial_open_port
    (
    SERIAL_PORT port
    )
{
/* Prompt for serial port */
char port_buf[12];
emulator_log("Please enter your serial port in the format /dev/ttyXX or in the format COMX.", EMULATOR_SUBSYSTEM_SERIAL);
printf("Input: \n");

if(fgets(port_buf, sizeof(port_buf), stdin) == NULL){
    emulator_log("Invalid port input.", EMULATOR_SUBSYSTEM_SERIAL);
    return false;
}

port_buf[strcspn(port_buf, "\n")] = '\0';

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
    snprintf(port_buf, sizeof(port_buf), "/dev/ttyS%u", last_two_digits);
    #pragma GCC diagnostic pop
}

serial_ports[ port ].port_handle = open(port_buf, O_RDWR | O_NOCTTY | O_NDELAY); // Open the port

if (serial_ports[ port ].port_handle < 0) {
    emulator_log("Error opening serial port.", EMULATOR_SUBSYSTEM_SERIAL);
    return false;
}

struct termios tty;

// Read in existing settings, handle errors
if( tcgetattr( serial_ports[ port ].port_handle, &tty ) != 0 ) {
    emulator_log("tcgetattr failed.", EMULATOR_SUBSYSTEM_SERIAL);
    return false;
}

// Configure port settings (baud rate, parity, etc.)
cfsetospeed( &tty, B921600 ); // Set output baud rate to 921600
cfsetispeed( &tty, B921600 ); // Set input baud rate to 921600

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
if (tcsetattr( serial_ports[ port ].port_handle, TCSANOW, &tty ) != 0) {
    emulator_log("tcsetattr failed.", EMULATOR_SUBSYSTEM_SERIAL);
    return false;
}

if (fcntl( serial_ports[ port ].port_handle, F_GETFD ) == -1) {
    emulator_log("FD became invalid.", EMULATOR_SUBSYSTEM_SERIAL);
    return false;
}
else {
    emulator_debug_logf("FD remails valid at end of init.", EMULATOR_SUBSYSTEM_SERIAL);
}

return true;

} /* emulator_prompt_and_open_serial_port_gs */


/**
* @brief Write to the virtual serial port.                                      
*/
void emulator_serial_write
    (
    SERIAL_PORT     port,           /* The serial port to write to     */
    const uint8_t*  tx_data_ptr,    /* Buffer to write from            */
    size_t          tx_data_size    /* Size of the data to write       */
    )
{
if ( serial_ports[ port ].port_handle < 0 )
    {
    return;
    }

write( serial_ports[ port ].port_handle, tx_data_ptr, tx_data_size );

} /* serial_write */


/**
 * @brief Read from the VCP
 * 
 * @param rx_data_ptr The buffer to read to
 * @param rx_data_size The size of the buffer
 *
 * @return The status of the read (cast to USB_STATUS)
 */
uint32_t emulator_serial_read
    (
    SERIAL_PORT port,               /* The serial port to read from    */
    void*    rx_data_ptr ,          /* Buffer to export data to        */
	size_t   rx_data_size           /* Size of the data to be received */
    )
{

// Verify fd is still valid before reading
if (fcntl(serial_ports[ port ].port_handle, F_GETFD) == -1) {
    emulator_log("Read: File descriptor became invalid.", EMULATOR_SUBSYSTEM_SERIAL);
    return USB_FAIL;
}

/* Blocking read */
struct timeval tv;
tv.tv_sec  = 0;
tv.tv_usec = 10000; // 10 ms

fd_set rfds;
FD_ZERO(&rfds);
FD_SET(serial_ports[ port ].port_handle, &rfds);

int ret = select(serial_ports[ port ].port_handle + 1, &rfds, NULL, NULL, &tv);
if (ret < 0) {
    emulator_log("Select failed.", EMULATOR_SUBSYSTEM_SERIAL);
    return USB_FAIL;
} else if (ret == 0) {
    return USB_TIMEOUT;
}

memset( rx_data_ptr, 0, rx_data_size );
int n = read( serial_ports[ port ].port_handle, rx_data_ptr, rx_data_size );
    
    if (n < 0) {
        emulator_log("Read failed.", EMULATOR_SUBSYSTEM_SERIAL);
        return USB_FAIL;
    } else if (n == 0) {
        return USB_TIMEOUT;
    } else {
        return USB_OK;
    }

} /* serial_read */


/**
 * @brief Detect whether the file descriptor is valid.
 * 
 * @param port The port to check.
 * @return true USB detected.
 * @return false USB not detected.
 */
bool emulator_usb_detect
    (
    SERIAL_PORT port
    )
{
return (serial_ports[ port ].port_handle != -1);

} /* emulator_usb_detect */