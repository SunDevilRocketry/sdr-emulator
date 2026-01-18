/*******************************************************************************
*
* FILE: 
* 		emulator_timer.c
*
* DESCRIPTION: 
* 		Mocks out HAL timers.
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
#include <time.h>
#include <stdint.h>
#include <stdio.h>

#include "emulator.h"

/*------------------------------------------------------------------------------
 Global Variables                                                     
------------------------------------------------------------------------------*/
uint64_t timers_start_time = 0;

/*------------------------------------------------------------------------------
 Static Procedure Prototypes                                                   
------------------------------------------------------------------------------*/

static uint64_t get_current_time
    (
    void
    );

/*------------------------------------------------------------------------------
 HAL interfaces                                                     
------------------------------------------------------------------------------*/

uint32_t HAL_GetTick() {
    return get_current_time - timers_start_time;
}

void HAL_Delay(uint32_t delay_time) {
    struct timespec req;
    req.tv_sec = delay_time / 1000; /* seconds */
    req.tv_nsec = (delay_time % 1000) * 1000000L; /* milliseconds to nanoseconds */
    nanosleep(&req, NULL);
}

/*------------------------------------------------------------------------------
 Procedures                                                     
------------------------------------------------------------------------------*/

/*******************************************************************************
*                                                                              *
* PROCEDURE:                                                                   * 
* 		emulator_start_timers                                                  *
*                                                                              *
* DESCRIPTION:                                                                 * 
*       Initializes the timers used by the emulator.                           *
*                                                                              *
*******************************************************************************/
void emulator_start_timers
    (
    void
    )
{
timers_start_time = get_current_time();

} /* emulator_start_timers */


/*******************************************************************************
*                                                                              *
* PROCEDURE:                                                                   * 
* 		get_current_time                                                       *
*                                                                              *
* DESCRIPTION:                                                                 * 
*       Gets the current time in milliseconds.                                 *
*                                                                              *
*******************************************************************************/
static uint64_t get_current_time
    (
    void
    )
{
struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);

    return (uint64_t)ts.tv_sec * 1000ULL +
           (uint64_t)ts.tv_nsec / 1000000ULL;  

} /* get_current_time */

