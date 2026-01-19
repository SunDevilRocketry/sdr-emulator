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
#include <stdint.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>

#include <stddef.h>

#include "emulator.h"
#include "stm32h755xx.h"

#include <stddef.h>

/*------------------------------------------------------------------------------
 Constants                                                       
------------------------------------------------------------------------------*/
#define GUI_PIPE "../../../../emulator/resources/gui-pipe"
const char DEVICE_ID[] = "SW_EMULATOR";

/*------------------------------------------------------------------------------
 Globals                                                       
------------------------------------------------------------------------------*/
int GUI_Pipe_FD;

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

void HAL_NVIC_DisableIRQ(IRQn_Type IRQn) {}
void HAL_NVIC_EnableIRQ(IRQn_Type IRQn) {}

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
 Open pipe for IPC                                                  
------------------------------------------------------------------------------*/
if (mkfifo(GUI_PIPE, 0666) == -1) {
        if (errno != EEXIST) {
            perror("Emulator Init: Could not create pipe.");
            return 1;
        }
        printf("Emulator Init: Named pipe '%s' already exists.\n", GUI_PIPE);
    } else {
        printf("Emulator Init: Named pipe '%s' created.\n", GUI_PIPE);
    }

GUI_Pipe_FD = open(GUI_PIPE, O_WRONLY | O_NONBLOCK);
if (GUI_Pipe_FD == -1) 
    {
    perror("open error");
    return 1;
    }
else
    {
    printf("Emulator Init: Pipe opened successfully!\n");
    }


/*------------------------------------------------------------------------------
 Start GUI and wait                                                  
------------------------------------------------------------------------------*/
pid_t pid;
pid = fork();

if ( pid < 0 ) 
    {
    fprintf(stderr, "Emulator Init: GUI Fork failed!\n");
    return 1;
    } 
else if ( pid == 0 ) 
    {
    execv("python ../../../../emulator/gui.py", NULL);
    exit(0);
    } 
else {
    printf("Emulator Init: GUI Fork success. Waiting for the GUI to initialize before continuing.\n");
    sleep(5);
    printf("Emulator Init: Continuing with startup.\n");
    }

/*------------------------------------------------------------------------------
 Once setup is complete, run the firmware                                                    
------------------------------------------------------------------------------*/
main_fut();

} /* main */


/*******************************************************************************
*                                                                              *
* PROCEDURE:                                                                   * 
* 		guipipe_put                                                            *
*                                                                              *
* DESCRIPTION:                                                                 * 
*       Write some string to the GUI pipe.                                     *
*                                                                              *
*******************************************************************************/
void guipipe_put
    (
    const char* message,
    size_t size
    )
{
write(GUI_Pipe_FD, message, size);

} /* guipipe_put */