/*******************************************************************************
*
* FILE: 
* 		emulator.c
*
* DESCRIPTION: 
* 		Large mock library for SDR hardware to allow builds of the firmware
*       on local hardware for testing,
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
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>

#include <stddef.h>

#include "emulator.h"
#include "stm32h755xx.h"

#include <stddef.h>
#include <pthread.h>

/*------------------------------------------------------------------------------
 Constants                                                       
------------------------------------------------------------------------------*/
const char DEVICE_ID[] = "SW_EMULATOR";
#define GUI_SOCK_PORT 5100

/*------------------------------------------------------------------------------
 Globals                                                       
------------------------------------------------------------------------------*/
int gui_sock_fd;
int gui_connection_fd;

extern volatile bool ignite_flag;
volatile bool irq_enabled = true;

/*------------------------------------------------------------------------------
 Static Prototypes                                                       
------------------------------------------------------------------------------*/
static void guisock_open
    (
    void
    );

static void* sock_listener
    (
    void* arg
    );

/*------------------------------------------------------------------------------
 HAL interfaces                                                       
------------------------------------------------------------------------------*/

uint32_t HAL_GetUIDw0(void) {
    uint32_t buf;
    memcpy(&buf, DEVICE_ID, 4);
    return buf;
}

uint32_t HAL_GetUIDw1(void) {
    uint32_t buf;
    memcpy(&buf, DEVICE_ID + 4, 4);
    return buf;
}

uint32_t HAL_GetUIDw2(void) {
    uint32_t buf;
    memcpy(&buf, DEVICE_ID + 8, 4);
    return buf;
}

/* checked by emulator_uart and emulator_i2c before mocking ISRs */
void HAL_NVIC_DisableIRQ(IRQn_Type IRQn) {irq_enabled = false;}
void HAL_NVIC_EnableIRQ(IRQn_Type IRQn) {irq_enabled = true;}

/*------------------------------------------------------------------------------
 Procedures                                                     
------------------------------------------------------------------------------*/

/*******************************************************************************
*                                                                              *
* PROCEDURE:                                                                   * 
* 		main                                                                   *
*                                                                              *
* DESCRIPTION:                                                                 * 
*       Emulator application entry point.                                      *
*                                                                              *
*******************************************************************************/
int main
    (
    void
    )
{
/*------------------------------------------------------------------------------
 Start software timers                                                    
------------------------------------------------------------------------------*/
emulator_start_timers();

/*------------------------------------------------------------------------------
 Check for flash and create the blank file if it doesn't exist                                                  
------------------------------------------------------------------------------*/
emulator_flash_init();

/*------------------------------------------------------------------------------
 Open socket for IPC                                                  
------------------------------------------------------------------------------*/
guisock_open();
printf("Emulator Init: GUI socket opened successfully.\n");

/*------------------------------------------------------------------------------
 Start GUI and wait                                                  
------------------------------------------------------------------------------*/
pid_t gui_pid;
gui_pid = fork();

if ( gui_pid < 0 ) 
    {
    fprintf(stderr, "Emulator Init: GUI Fork failed!\n");
    return 1;
    } 
else if ( gui_pid == 0 ) 
    {
    execlp("python", "python", "../../../../emulator/gui/gui.py", (char *) NULL);
    exit(0);
    } 
else {
    printf("Emulator Init: GUI Fork success. Waiting for the GUI to initialize before continuing.\n");
    sleep(4);
    printf("Emulator Init: Continuing with startup.\n");
    }

/*------------------------------------------------------------------------------
 Attempt to open connection                                                  
------------------------------------------------------------------------------*/
struct sockaddr_in client_address;
socklen_t addrlen = sizeof(client_address);

while ( (gui_connection_fd = accept(gui_sock_fd, (struct sockaddr *)&client_address, &addrlen) ) < 0) 
    {
    printf("Emulator Init: Socket accept failed.\n");
    printf("Emulator Init: Retrying...\n");
    sleep(1);
    } 
printf("Emulator Init: Socket connected to GUI client.\n");
printf("    [DEBUG] IP: %s\n", inet_ntoa(client_address.sin_addr));
printf("    [DEBUG] Port: %d\n", ntohs(client_address.sin_port));

printf("Emulator Init: Opening socket listener.\n");
pthread_t socket_thread;
pthread_create( &socket_thread, NULL, sock_listener, NULL );

printf("Emulator Init: Opening I2c interrupt listener.\n");
pthread_t it_thread;
pthread_create( &it_thread, NULL, emulator_i2c_it_listener, NULL );

/*------------------------------------------------------------------------------
 Register Default Error Callback                                                   
------------------------------------------------------------------------------*/
printf("Emulator Init: Registering default error handler.\n");
emulator_setup_error();

/*------------------------------------------------------------------------------
 Select COM port
------------------------------------------------------------------------------*/
if ( emulator_prompt_and_open_serial_port() )
    {
    printf("Emulator Init: Serial connection OK.\n");
    }
else
    {
    printf("Emulator Init: Serial connection failed. Continuing without.\n");
    }

/*------------------------------------------------------------------------------
 Once setup is complete, run the firmware                                                    
------------------------------------------------------------------------------*/
printf("Emulator Init: Starting firmware.\n");
sleep(10);
main_fut();

} /* main */


/*******************************************************************************
*                                                                              *
* PROCEDURE:                                                                   * 
* 		guisock_open                                                           *
*                                                                              *
* DESCRIPTION:                                                                 * 
*       Start the GUI socket.                                                  *
*                                                                              *
*******************************************************************************/
static void guisock_open
    (
    void
    )
{

struct sockaddr_in sock_addr; 

if ( ( gui_sock_fd = socket( AF_INET, SOCK_STREAM, 0 ) ) < 0 ) 
    {
    perror("Emulator Init: Error while creating the socket\n");
    exit(1);
    }

memset( &sock_addr, 0, sizeof(sock_addr) ); 

sock_addr.sin_family = AF_INET;

sock_addr.sin_port = htons( GUI_SOCK_PORT );

sock_addr.sin_addr.s_addr = htonl( INADDR_ANY ); /* allow the emulator to accept a connection on any interface */

if ( bind( gui_sock_fd, (struct sockaddr *) &sock_addr, sizeof( sock_addr ) ) < 0 ) 
    {
    perror("Emulator Init: Error in binding GUI socket.\n");
    exit(1);
    } 

if (listen(gui_sock_fd, 1) < 0) {
    perror("Emulator Init: Socket listen failed.\n");
    exit(1);
}
printf("Emulator Init: Socket is now listening on port %d.\n", GUI_SOCK_PORT);

} /* guisock_open */


/*******************************************************************************
*                                                                              *
* PROCEDURE:                                                                   * 
* 		sock_listener                                                          *
*                                                                              *
* DESCRIPTION:                                                                 * 
*       Listen on the socket for events.                                       *
*                                                                              *
*******************************************************************************/
static void* sock_listener
    (
    void* arg
    )
{
bool sock_listen = true;
char buf[256];

printf("Emulator Init: Socket listener running.\n");

while (sock_listen)
    {
    /* Read up to 256 bytes from the socket at a time */
    int bytes_read = 0;
    bytes_read = read( gui_connection_fd, buf, sizeof( buf ) );

    /* If there's content in the socket, tokenize it based on newlines*/
    if ( bytes_read > 0 )
        {
        char* token = strtok(buf, "\n");

        while ( token != NULL )
            {
            /* Handle each command here */
            if ( !strncmp( token, "ignite", 6 ) )
                {
                ignite_flag = true;
                }

            token = strtok( NULL, "\n" );
            }
        }
    /* Do not busy wait. Delay to allow the buffer to refill. */
    usleep(100000); /* 100000 microseconds -> 100 ms */
    }

    return 0;

} /* sock_listener */


/*******************************************************************************
*                                                                              *
* PROCEDURE:                                                                   * 
* 		guisock_put                                                            *
*                                                                              *
* DESCRIPTION:                                                                 * 
*       Write some string to the GUI socket.                                   *
*                                                                              *
*******************************************************************************/
void guisock_put
    (
    const char* message,
    size_t size
    )
{
write( gui_connection_fd, message, size );

} /* guisock_put */