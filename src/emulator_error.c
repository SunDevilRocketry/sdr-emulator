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

#if defined(__GLIBC__)
#include <execinfo.h>
#elif defined( _WIN32 ) || defined( __CYGWIN__ )
#include <windows.h>
#include <DbgHelp.h>
#else
#warning "Your platform does not support backtraces. Please contribute this feature or report your toolchain information."
#endif

#include "emulator.h"

#include "error_sdr.h"

/*------------------------------------------------------------------------------
 Constants                                                       
------------------------------------------------------------------------------*/
#define MAX_FRAMES 128

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

static void print_stack_trace
    (
    void
    );

/*------------------------------------------------------------------------------
 Procedures                                                     
------------------------------------------------------------------------------*/

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

} /* emulator_setup_error */


/*******************************************************************************
*                                                                              *
* PROCEDURE:                                                                   * 
* 		emulator_error_handler                                                 *
*                                                                              *
* DESCRIPTION:                                                                 * 
*       Start the GUI socket.                                                  *
*                                                                              *
*******************************************************************************/
static void emulator_error_handler
    (
    ERROR_CODE error_code
    )
{
printf( "\nEmulator: A terminal error has been reached.\n" );
printf( "    [DEBUG]: Integer error code - %d.\n", error_code );
print_stack_trace();
printf( "\nEmulator: The emulator will now exit.\n");
exit(0);

} /* emulator_error_handler */


/*******************************************************************************
*                                                                              *
* PROCEDURE:                                                                   * 
* 		print_stack_trace                                                      *
*                                                                              *
* DESCRIPTION:                                                                 * 
*       Print the current call stack to the console.                           *
*                                                                              *
*******************************************************************************/
static void print_stack_trace
    (
    void
    ) 
{
#if defined(__GLIBC__)
void* callstack[MAX_FRAMES];
int frames, i;
char** strs;

frames = backtrace(callstack, MAX_FRAMES);
strs = backtrace_symbols(callstack, frames);

if (strs == NULL) {
    perror("Emulator: A backtrace could not be completed.");
    exit(EXIT_FAILURE);
}

printf("Emulator: Stack trace (max %d frames):\n", MAX_FRAMES);
for (i = 0; i < frames; ++i) {
    printf("%s\n", strs[i]);
}

free(strs);
#elif defined( _WIN32 ) || defined( __CYGWIN__ )
/**
 * The following code is exempt from software licensing on this project.
 * This is not original code and Sun Devil Rocketry does not claim ownership.
 * Source:
 * 
 * https://stackoverflow.com/questions/5693192/win32-backtrace-from-c-code
 */
unsigned int   i;
void         * stack[ 100 ];
unsigned short frames;
SYMBOL_INFO  * symbol;
HANDLE         process;

process = GetCurrentProcess();

SymInitialize( process, NULL, TRUE );

frames               = CaptureStackBackTrace( 0, 100, stack, NULL );
symbol               = ( SYMBOL_INFO * )calloc( sizeof( SYMBOL_INFO ) + 256 * sizeof( char ), 1 );
symbol->MaxNameLen   = 255;
symbol->SizeOfStruct = sizeof( SYMBOL_INFO );

printf( "Emulator: Stack trace:\n" );
for( i = 0; i < frames; i++ )
    {
    SymFromAddr( process, ( DWORD64 )( stack[ i ] ), 0, symbol );

    printf( "%i: %s - 0x%0llX\n", frames - i - 1, symbol->Name, symbol->Address );
    }

free( symbol );

printf( "[ETS TEMP]: The stack trace for windows builds is very imperfect. Sorry!\n" );
#else
printf("Emulator: Your platform does not support backtraces for errors.\n");
#endif
} /* print_stack_trace */