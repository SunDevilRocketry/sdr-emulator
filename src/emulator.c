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

#include <stddef.h>
#include <pthread.h>

/*------------------------------------------------------------------------------
 Constants                                                       
------------------------------------------------------------------------------*/
const char DEVICE_ID[] = "SW_EMULATOR";

/*------------------------------------------------------------------------------
 Globals                                                       
------------------------------------------------------------------------------*/

extern volatile bool ignite_flag;

/*------------------------------------------------------------------------------
 Static Prototypes                                                       
------------------------------------------------------------------------------*/

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
 Seed RNG for noise generator                                                 
------------------------------------------------------------------------------*/
srand(time(NULL));

/*------------------------------------------------------------------------------
 Open socket for IPC                                                  
------------------------------------------------------------------------------*/
/*
guisock_open();
printf("Emulator Init: GUI socket opened successfully.\n");
*/

/*------------------------------------------------------------------------------
 Start GUI and wait                                                  
------------------------------------------------------------------------------*/
/* GLFW is big & greedy, and many of its functions must be called on main thread */
pid_t gui_pid;
gui_pid = fork();

if ( gui_pid < 0 ) 
    {
    fprintf(stderr, "Emulator Init: GUI Fork failed!\n");
    return 1;
    } 
else if ( gui_pid == 0 ) 
    {
    // Call glfw stuffies
    emulator_gui_main();
    } 
else {
    printf("Emulator Init: GUI Fork success. Waiting for the GUI to initialize before continuing.\n");
    sleep(4);
    printf("Emulator Init: Continuing with startup.\n");
    }

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


