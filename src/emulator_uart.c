/*******************************************************************************
*
* FILE: 
* 		emulator_uart.c
*
* DESCRIPTION: 
* 		Mocks the functionality of UART peripherals on the FC.
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

/* POSIX */
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>

/*------------------------------------------------------------------------------
 Globals                                                       
------------------------------------------------------------------------------*/
int serial_port = -1;

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
 HAL interfaces                                                       
------------------------------------------------------------------------------*/

HAL_StatusTypeDef HAL_UART_Transmit(UART_HandleTypeDef *huart, const uint8_t *pData, uint16_t Size, uint32_t Timeout) {
    if ( huart == &(USB_HUART) )
        {
        serial_write( pData, (size_t)Size );
        }
    return HAL_OK;
}

HAL_StatusTypeDef HAL_UART_Receive(UART_HandleTypeDef *huart, uint8_t *pData, uint16_t Size, uint32_t Timeout) {
    if ( huart == &(USB_HUART) )
        {
        serial_read( pData, (size_t)Size );
        }
    return HAL_OK;
}

HAL_StatusTypeDef HAL_UART_Receive_IT(UART_HandleTypeDef *huart, uint8_t *pData, uint16_t Size) {
    return HAL_OK;
}

/*------------------------------------------------------------------------------
 Procedures                                                     
------------------------------------------------------------------------------*/

bool emulator_prompt_and_open_serial_port
    (
    void
    )
{
/* Prompt for serial port */
char port_buf[12];
printf("Serial Init: Please enter your serial port in the format /dev/ttyXX or in the format COMX.\n\n");
#if defined( _WIN32 ) || defined( __CYGWIN__ )
printf("    Note for Windows users:\n");
printf("    COM1 maps to /dev/ttyS0\n");
printf("    COM2 maps to /dev/ttyS1\n");
printf("    ETS Temp: use /dev/ttyS0\n");
printf("    (etc)\n");
#endif
printf("Input: \n");

if(fgets(port_buf, sizeof(port_buf), stdin) == NULL){
    perror("Serial Init: invalid port input\n");
    return false;
}

port_buf[strcspn(port_buf, "\n")] = 0;

char com_buf[4];
int com_port_num;

strncpy(com_buf, port_buf, 3);

if(strcmp(com_buf, "COM") == 0){
    sscanf(port_buf+3, "%d", &com_port_num);
    com_port_num--;
    snprintf(port_buf, 12, "/dev/ttyS%d", com_port_num);
}

serial_port = open(port_buf, O_RDWR | O_NOCTTY | O_NDELAY); // Open the port

if (serial_port < 0) {
    perror("Serial Init: error opening serial port\n");
    return false;
}

struct termios tty;

// Read in existing settings, handle errors
if(tcgetattr(serial_port, &tty) != 0) {
    perror("Serial Init: tcgetattr failed.\n");
    return false;
}

// Configure port settings (baud rate, parity, etc.)
cfsetospeed(&tty, B921600); // Set output baud rate to 9600
cfsetispeed(&tty, B921600); // Set input baud rate to 9600

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
if (tcsetattr(serial_port, TCSANOW, &tty) != 0) {
    perror("Serial Init: tcsetattr failed.\n");
    return false;
}

if (fcntl(serial_port, F_GETFD) == -1) {
    fprintf(stderr, "Serial Init: File descriptor became invalid\n");
    return false;
}
else {
    printf("Serial Init: File descriptor is still valid at end of init\n");
}

return true;

}


static void serial_write
    (
    const uint8_t* msg,
    size_t len
    )
{
if ( serial_port < 0 )
    {
    return;
    }

write( serial_port, msg, len );

} /* serial_write */


static USB_STATUS serial_read
    (
    void*    rx_data_ptr , /* Buffer to export data to        */
	size_t   rx_data_size  /* Size of the data to be received */
    )
{

// Verify fd is still valid before reading
if (fcntl(serial_port, F_GETFD) == -1) {
    fprintf(stderr, "Serial: Read: File descriptor became invalid\n");
    return USB_FAIL;
}

/* Blocking read */
struct timeval tv;
tv.tv_sec  = 0;
tv.tv_usec = 10000; // 10 ms

fd_set rfds;
FD_ZERO(&rfds);
FD_SET(serial_port, &rfds);

int ret = select(serial_port + 1, &rfds, NULL, NULL, &tv);
if (ret < 0) {
    perror("Serial: Select Failed");
    return USB_FAIL;
} else if (ret == 0) {
    return USB_TIMEOUT;
}

memset( rx_data_ptr, 0, rx_data_size );
int n = read( serial_port, rx_data_ptr, rx_data_size );
    
    if (n < 0) {
        perror("Serial: Read Failed");
        return USB_FAIL;
    } else if (n == 0) {
        return USB_TIMEOUT;
    } else {
        return USB_OK;
    }

} /* serial_read */