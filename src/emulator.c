/**
* @file emulator.c 
* 
*
* Large mock library for SDR hardware to allow builds of the firmware
* on local hardware for testing,
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
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
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

/*------------------------------------------------------------------------------
 Static Variables
------------------------------------------------------------------------------*/

static pthread_t firmware_thread;
static pthread_t it_thread;
static pthread_t gps_thread;
static pthread_t gs_thread;

static EMULATOR_FLAGS_TYPE emulator_flags = IRQ_ENABLED_FLAG_BIT | GUI_ENABLED_FLAG_BIT;

/*------------------------------------------------------------------------------
 Static Functions
------------------------------------------------------------------------------*/

/**
* Callback function to handle SIGINTs and SIGTERMs from the user. Wraps @ref emulator_exit
* @param dummy Dummy parameter to match the callback's expected signature.
* @warning Do not call this function by itself.
*/
static void sigint_handler
    (
    int dummy
    )
{

emulator_exit(0);
} /* sigint_handler */

/**
* Helper function which prints information detailing how to use th emulator from the command line.
*/
static void print_args_help
    (
    void
    ) 
{

printf("Usage: build/appa [OPTION]\n");
printf("Runs the flight computer emulator\n\n");
printf("\t-h, --help              Displays this screen and exits\n");
printf("\t--no-gui                Runs the emulator without the GUI (CLI only)\n");
} /* pring_args_help */

/**
* Parses command line arguments.
* @param argc The number of arguments
* @param argv Array of arguments
*/
static void parse_args
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
        { "debug", no_argument, NULL, 0}, /* For prints such as address writing */
        { "fast-arm", no_argument, NULL, 0} /* Arms the FC immediately on startup */

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
                emulator_flags_unset_bits(GUI_ENABLED_FLAG_BIT);
                }
            else if ( option_index == 1 )
                {
                print_args_help();
                exit(0);
                }
            else if ( option_index == 4 )
                {
                emulator_flags_set_bits(IGNITE_FAST_ARM_FLAG_BIT);
                }
            break;

        case 'h':
            print_args_help();
            exit(0);
            break;

        case '?':
            printf("Unknown argument -%c\n", optopt);
            print_args_help();
            exit(0);
            break;
        }
    }

} /* parse_args */

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
void HAL_NVIC_DisableIRQ(IRQn_Type IRQn) {emulator_flags_unset_bits(IRQ_ENABLED_FLAG_BIT);}
void HAL_NVIC_EnableIRQ(IRQn_Type IRQn) {emulator_flags_set_bits(IRQ_ENABLED_FLAG_BIT);}

/*------------------------------------------------------------------------------
 Procedures                                                     
------------------------------------------------------------------------------*/

/**
* Emulator application entry point.                                      
*/                                                                              
int main
    (
    int argc,
    char* const argv[]
    )
{

parse_args(argc, argv);

/* Connect sigint handler */
signal(SIGINT, sigint_handler);
signal(SIGTERM, sigint_handler);

/* Start software timers */
emulator_start_timers();

/* Check for flash and create the blank file if it doesn't exist */                                                
emulator_flash_init();

/* Seed RNG for noise generator */
srand(time(NULL));

emulator_log("Opening I2c interrupt listener.", EMULATOR_SUBSYSTEM_INIT);
pthread_create( &it_thread, NULL, emulator_i2c_it_listener, NULL );

emulator_log("Opening GPS interrupt listener.", EMULATOR_SUBSYSTEM_INIT);
pthread_create( &gps_thread, NULL, emulator_gps_it_listener, NULL );

/*------------------------------------------------------------------------------
 Register Default Error Callback                                                   
------------------------------------------------------------------------------*/
emulator_log("Registering default error handler.", EMULATOR_SUBSYSTEM_INIT);
emulator_setup_error();

 /* Select COM port */
if ( emulator_serial_open_port( FC_SERIAL_PORT ) )
    {
    emulator_log("FC Serial connection OK.", EMULATOR_SUBSYSTEM_INIT);
    }
else
    {
    emulator_log("FC Serial connection failed -- continuing without.", EMULATOR_SUBSYSTEM_INIT);
    }

/* Select COM port -- ground station */
if ( emulator_serial_open_port( GS_SERIAL_PORT ) )
    {
    emulator_log("GS Serial connection OK.", EMULATOR_SUBSYSTEM_INIT);
    pthread_create( &gs_thread, NULL, emulator_gs_terminal_loop, NULL );
    }
else
    {
    emulator_log("GS Serial connection failed -- continuing without.", EMULATOR_SUBSYSTEM_INIT);
    }

/* Initialize GUI/Firmware */
if ( emulator_flags_check_bits(GUI_ENABLED_FLAG_BIT) ) 
    {
    /*------------------------------------------------------------------------------
     Once setup is complete, run the firmware                                                    
    ------------------------------------------------------------------------------*/
    emulator_log("Starting firmware.", EMULATOR_SUBSYSTEM_INIT);

    /* Ugly cast to correct function type (might be the worst cast I've ever seen) */
    /* Shouldn't happen in normal execution, but if main_fut returns, likely UB */
    pthread_create( &firmware_thread, NULL, (void*(*)(void*))main_fut, NULL );

    /* Run and block until GUI termination */
    emulator_gui_main();

    }
else 
    {
    /* If GUI disabled, run firmware directly */
    main_fut();
    }

emulator_exit(EXIT_SUCCESS);
} /* main */

/**
 *
 * Cleans up emulator state and exits the program
 *
 * @param exitCode The exit code passed to exit()
 */
void emulator_exit
    (
    int exitCode
    )
{
emulator_logf("Emulator terminating with exit code %d.", EMULATOR_SUBSYSTEM_GUI_INFO, exitCode);
if ( emulator_flags_check_bits(GUI_ENABLED_FLAG_BIT) )
    {
    emulator_gui_teardown();
    }

/* Make sure cov data is written */
fflush(NULL);

/* Should force kill all pthreads */
exit(exitCode);

}

/**
 * Bitwise ORs the passed flags with the flag bitfield
 *
 * @param flags The list of flags to set.
 * @note All flags passed to flags will be set in the emulator flags
 */
void emulator_flags_set_bits
    (
    EMULATOR_FLAGS_TYPE flags
    )
{
emulator_flags |= flags;

}

/**
* Bitwise ANDs the negation of the passed flags to set the passed flag bits to zero
*
* @param flags The list of flags to unset
* @note All flags passed to flags will be unset in the emulator flags
*/
void emulator_flags_unset_bits
    (
    EMULATOR_FLAGS_TYPE flags
    )
{
emulator_flags &= ~flags;

}

/**
* Iterative helper function for @ref emulator_flags_check_bits
*
* @note Should only be called by @ref emulator_flags_check_bits
*/
static bool emulator_flags_check_bits_iter
    (
     EMULATOR_FLAGS_TYPE flags,
     int bit_index,
     EMULATOR_FLAGS_TYPE runner
    )
{
/* Isloates bit at bit_index */
EMULATOR_FLAGS_TYPE mask = (flags & (~flags | 1u << bit_index));
/* Check if bit is set in emulator_flags */
bool thisFlag = emulator_flags & mask;
/* If not set, and there are no more flags, return false early */
if ( (emulator_flags & mask) == 0 && (flags & mask) == 1)
    {
    return false;

    }

/* Set flag bit in runner */
runner |= (thisFlag << bit_index);

if (bit_index != 0)
    {
    /* Perform next iteration if there are more bits to go */
    return emulator_flags_check_bits_iter(flags, --bit_index, runner);

    } 
else
    {
    /* Check if runner and flags are exactly equal to flags */
    return (flags & runner) == flags;

    }
}

/*
* Returns TRUE ONLY IF all passed flags are enabled internally, else false
*
* @param flags Bitfield of flags to check
*/
bool emulator_flags_check_bits
    (
    EMULATOR_FLAGS_TYPE flags
    )
{

if ( (emulator_flags & flags) == 0 )
    {
        return false;
    }

return emulator_flags_check_bits_iter(flags, (sizeof(EMULATOR_FLAGS_TYPE) * 8) - 1, 0);
}



