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

#include "emulator.h"
#include "error_sdr.h"
#include "debug_sdr.h"

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
char log_msg[DEBUG_MSG_MAX_LEN + 32] = "[";
size_t new_len = 1;
if( from_subsystem != NULL )
    {
    new_len += strlcpy(log_msg + 1, from_subsystem, 29);
    }
else
    {
    new_len += strlcpy(log_msg + 1, "EMULATOR", 29);
    }
log_msg[new_len] = ']';
log_msg[new_len + 1] = ' ';
new_len += 2;

// Override newline logic to make sure we have it here
if( msg_len > 0 && msg[msg_len - 1] == '\n' )
    {
    msg_len--;
    }
memcpy(log_msg + new_len, msg, msg_len);
new_len += msg_len;
log_msg[new_len] = '\n';
new_len++;
fwrite(log_msg, 1, (int)new_len, stdout);
fflush(stdout); /* put and flush immediately */

// ETS TODO: Log to subsystem file

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
emulator_log("The emulator has encountered a terminal error and will now exit.\n", "ERROR-HANDLER");
error_len = snprintf(error_msg, 64, "Provided error code: %d\n", error_code);
emulator_debug_log(error_msg, error_len, "ERROR-HANDLER" );
exit(0);

} /* emulator_error_handler */