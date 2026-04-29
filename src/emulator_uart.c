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
extern volatile bool      irq_enabled;
extern uint8_t            gps_mesg_byte;
extern uint8_t            rx_buffer[GPSBUFSIZE];
extern GPS_DATA           gps_data;

int serial_port = -1;
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
        serial_write( pData, (size_t)Size );
        }
    return HAL_OK;
}

HAL_StatusTypeDef HAL_UART_Transmit_IT(UART_HandleTypeDef *huart, const uint8_t *pData, uint16_t Size) {
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

/*******************************************************************************
*                                                                              *
* PROCEDURE:                                                                   * 
* 		emulator_prompt_and_open_serial_port                                   *
*                                                                              *
* DESCRIPTION:                                                                 * 
*       Set up the serial connection to SDEC.                                  *
*                                                                              *
*******************************************************************************/
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

} /* emulator_prompt_and_open_serial_port */


/*******************************************************************************
*                                                                              *
* PROCEDURE:                                                                   * 
* 		serial_write                                                           *
*                                                                              *
* DESCRIPTION:                                                                 * 
*       Write to the virtual serial port.                                      *
*                                                                              *
*******************************************************************************/
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


/*******************************************************************************
*                                                                              *
* PROCEDURE:                                                                   * 
* 		serial_read                                                            *
*                                                                              *
* DESCRIPTION:                                                                 * 
*       Read from the virtual serial port.                                     *
*                                                                              *
*******************************************************************************/
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


/*******************************************************************************
*                                                                              *
* PROCEDURE:                                                                   * 
* 		emulator_gps_it_listener                                               *
*                                                                              *
* DESCRIPTION:                                                                 * 
*       Listen for and fulfill UART GPS IT I/O.                                *
*                                                                              *
*******************************************************************************/
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

    if (irq_enabled) 
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


/*******************************************************************************
*                                                                              *
* PROCEDURE:                                                                   * 
* 		gps_read_handler_IT                                                    *
*                                                                              *
* DESCRIPTION:                                                                 * 
*       Interrupt the main thread with a new GPS message.                      *
*                                                                              *
*******************************************************************************/
static void gps_read_handler_IT
    (
    int message_num
    )
{
// memset( gps_data_ptr, 0, gps_data_size );
if (message_num >= array_size( gps_msgs ) )
    {
    printf("[GPS DEBUG] Error: index out of range\n");
    return;
    }
memcpy( rx_buffer, gps_msgs[message_num], strlen( gps_msgs[message_num] ) );

/* Pasted in from UART4_IRQHandler */
if(gps_mesg_validate((char*) rx_buffer))
    GPS_parse(&gps_data, (char*) rx_buffer);

memset(rx_buffer, 0, sizeof(rx_buffer));

gps_receive_IT(&gps_mesg_byte, 1);

} /* gps_read_handler_IT */
