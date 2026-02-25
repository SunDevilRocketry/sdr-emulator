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
#include <signal.h>

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
 Static Variables
------------------------------------------------------------------------------*/
static pthread_t firmwareThread;
static pthread_t it_thread;
/*------------------------------------------------------------------------------
 Static Functions
------------------------------------------------------------------------------*/
void sigintHandler
    (
    int dummy
    )
{
    emulator_exit(0);
}
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
 Connect sigint handler
------------------------------------------------------------------------------*/
signal(SIGINT, sigintHandler);

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


// [X] Make the main thread handle the gui and use a pthread for the emulator
// This lets me use shared memory to communicate
// [X] emulator_exit function that will ensure teardown before exiting program
// [X] Kill all pthreads when gui closes
// [X] Sigint handler to kill thread from console


printf("Emulator Init: Opening I2c interrupt listener.\n");
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
 Initialize GUI
------------------------------------------------------------------------------*/

emulator_gui_init();

/*------------------------------------------------------------------------------
 Once setup is complete, run the firmware                                                    
------------------------------------------------------------------------------*/
printf("Emulator Init: Starting firmware.\n");
//sleep(10);

// Ugly cast to correct function type (might be the worst cast I've ever seen)
pthread_create( &firmwareThread, NULL, (void*(*)(void*))main_fut, NULL );

/*------------------------------------------------------------------------------
 Run and block until GUI termination
------------------------------------------------------------------------------*/
emulator_gui_main();

emulator_exit(EXIT_SUCCESS);
} /* main */

void emulator_exit
    (
    int exitCode
    )
{

printf("Emulator terminating with exit code %d", exitCode);
emulator_gui_teardown();

/* Should force kill all pthreads */
exit(exitCode);

}


