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

#include "emulator.h"
#include "stm32h755xx.h"

/*------------------------------------------------------------------------------
 Constants                                                       
------------------------------------------------------------------------------*/
const char DEVICE_ID[] = "SW_EMULATOR";

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
 Once setup is complete, run the firmware                                                    
------------------------------------------------------------------------------*/
main_fut();

} /* main */