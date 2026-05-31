/*******************************************************************************
*
* FILE: 
* 		emulator_error.c
*
* DESCRIPTION: 
* 		Procedures related to error handling in emulated applications.
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
#include <stdio.h>
#include <stddef.h>
#include <stdarg.h>

#include "emulator.h"
#include "error_sdr.h"
#include "debug_sdr.h"
#include "timer.h"

/*------------------------------------------------------------------------------
 Constants                                                       
------------------------------------------------------------------------------*/
#define MAX_FRAMES 128
#define LOGGING_DIRECTORY "../../emulator/logs/"

/*------------------------------------------------------------------------------
 Globals                                                       
------------------------------------------------------------------------------*/
extern volatile ERROR_CALLBACK default_error_handler;

/*------------------------------------------------------------------------------
 Static Prototypes                                                       
------------------------------------------------------------------------------*/
static void emulator_error_handler
    (
    ERROR_CODE error_code
    );

/*------------------------------------------------------------------------------
 Procedures                                                     
------------------------------------------------------------------------------*/

/*******************************************************************************
*                                                                              *
* PROCEDURE:                                                                   * 
* 		emulator_debug_log                                                     *
*                                                                              *
* DESCRIPTION:                                                                 * 
*       Log to the console.                                                    *
*                                                                              *
*******************************************************************************/
void emulator_debug_log
    (
    const char* msg,
    size_t msg_len,
    const char* from_subsystem /* name of subsystem that logged the message */
    )
{
/* We have RAM privlieges so we can ignore the firmware's feeble 'max message size' */
(void)msg_len;

char* fmt_string = "[%02u:%02u:%02u:%03u] [%s] %s\n";

if ( from_subsystem == NULL )
    {
    from_subsystem = "EMULATOR";
    }
// TODO: Make debug_writer is main.c use the firmware subsystem macro
// I'm not doing that rn because I dont want to make a second pr :P
else if ( strcmp(EMULATOR_SUBSYSTEM_FIRMWARE, from_subsystem) == 0 )
    {
        // Skip the timestamp in the preamble
        // This will have to change if the preamble format changes
        size_t offset = sizeof("[XX:XX:XX.XXX");
        msg += offset;
        fmt_string = "[%02u:%02u:%02u:%03u] [%s] [%s\n";
    }

SYSTEM_TIME curr_time = get_system_time();

/* +1 for null terminator */
size_t fmt_size = snprintf
                    (
                    NULL, 
                    0, 
                    fmt_string, 
                    curr_time.hours,
                    curr_time.mins,
                    curr_time.secs,
                    curr_time.millis,
                    from_subsystem,
                    msg
                    ) + 1;

char* msg_buf = malloc(sizeof(char) * fmt_size);

if ( msg_buf == NULL )
    {
    const char failed_str[] = "Log message allocation failed\n";
    fwrite(failed_str, sizeof(char), sizeof(failed_str), stdout);
    return;
    }

snprintf
    (
    msg_buf, 
    fmt_size, 
    fmt_string, 
    curr_time.hours,
    curr_time.mins,
    curr_time.secs,
    curr_time.millis,
    from_subsystem,
    msg
    );

fwrite(msg_buf, sizeof(char), fmt_size, stdout);
fflush(stdout); /* put and flush immediately */

free(msg_buf);
}

void emulator_debug_logf
    (
    const char* msg,
    const char* from_subsystem, 
    ...
    )
{
va_list vargs;
va_start(vargs, from_subsystem); 

size_t msg_len = vsnprintf(NULL, 0, msg, vargs) + 1; 
char* new_msg = malloc(sizeof(char) * msg_len); 

if (new_msg == NULL)
{
    va_end(vargs);
    const char failed_str[] = "Log message allocation failed\n";
    fwrite(failed_str, sizeof(char), sizeof(failed_str), stdout);
    return;
}

vsnprintf(new_msg, msg_len, msg, vargs); 
emulator_debug_log( new_msg, msg_len, from_subsystem ); 

free(new_msg);

va_end(vargs);

}

/*******************************************************************************
*                                                                              *
* PROCEDURE:                                                                   * 
* 		emulator_setup_error                                                   *
*                                                                              *
* DESCRIPTION:                                                                 * 
*       Set up the error handler on the emulator.                              *
*                                                                              *
*******************************************************************************/
void emulator_setup_error
    (
    void
    )
{
default_error_handler = (ERROR_CALLBACK){ 0, emulator_error_handler };

/* clear last logs */
// ETS TODO
} /* emulator_setup_error */


/*******************************************************************************
*                                                                              *
* PROCEDURE:                                                                   * 
* 		emulator_error_handler                                                 *
*                                                                              *
* DESCRIPTION:                                                                 * 
*       Handle an error in the emulator.                                       *
*                                                                              *
*******************************************************************************/
static void emulator_error_handler
    (
    ERROR_CODE error_code
    )
{
char error_msg[64];
size_t error_len = 0;
emulator_log("The emulator has encountered a fatal error and will now exit.\n", "ERROR-HANDLER");
error_len = snprintf(error_msg, 64, "Provided error code: %d\n", error_code);
emulator_debug_log(error_msg, error_len, "ERROR-HANDLER" );
exit(0);

} /* emulator_error_handler */