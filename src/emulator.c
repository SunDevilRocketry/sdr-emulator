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
#include <getopt.h>

#include <stddef.h>

#include "emulator.h"

#include "stm32h755xx.h"

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
volatile bool irq_enabled = true;
volatile bool gui_enable = true;

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

static void printArgsHelp
    (
    void
    ) 
{

printf("Usage: build/appa [OPTION]\n");
printf("Runs the flight computer emulator\n\n\n");
printf("\t-h, --help              Displays this screen and exits\n");
printf("\t--no-gui                Runs the emulator without the GUI (CLI only)\n");

}

static void parseArgs
    (
    int argc, 
    char* const argv[]
    )
{

while (1) 
{
    int c;
    int option_index = 0;
    static struct option long_options[] = 
    {
        { "no-gui", no_argument, NULL, 0 }, /* Disables GUI */
        { "help", no_argument, NULL, 0}, /* help me */
        { "verbose", no_argument, NULL, 0}, /* For misc info like emulator initialized X system */
        { "debug", no_argument, NULL, 0} /* For prints such as address writing */

    };

    c = getopt_long(argc, argv, "-:h", long_options, &option_index);
    if (c == -1)
        {
        break;
        }

    switch (c)
        {
        case 0:
            if ( option_index == 0 )
                {
                gui_enable = false;
                }
            else if ( option_index == 1 )
                {
                printArgsHelp();
                exit(0);
                }
            break;

        case 'h':
            printArgsHelp();
            exit(0);
            break;

        case '?':
            printf("Unknown argument -%c\n", optopt);
            printArgsHelp();
            exit(0);
            break;
        }
}

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
    int argc,
    char* const argv[]
    )
{
// --no-gui command line argument

parseArgs(argc, argv);
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

if ( gui_enable ) 
{

emulator_gui_init();

/*------------------------------------------------------------------------------
 Once setup is complete, run the firmware                                                    
------------------------------------------------------------------------------*/
printf("Emulator Init: Starting firmware.\n");
//sleep(10);

// Ugly cast to correct function type (might be the worst cast I've ever seen)
// Shouldn't happen in normal execution, but if main_fut returns, likely UB
pthread_create( &firmwareThread, NULL, (void*(*)(void*))main_fut, NULL );

/*------------------------------------------------------------------------------
 Run and block until GUI termination
------------------------------------------------------------------------------*/
emulator_gui_main();

}
else 
{
main_fut();
}

emulator_exit(EXIT_SUCCESS);
} /* main */

void emulator_exit
    (
    int exitCode
    )
{

printf("Emulator terminating with exit code %d", exitCode);
if ( gui_enable ) 
{
    emulator_gui_teardown();
}

/* Should force kill all pthreads */
exit(exitCode);

}


