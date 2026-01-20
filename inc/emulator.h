/*******************************************************************************
*
* FILE: 
* 		emulator.h
*
* DESCRIPTION: 
* 		Main header file for SDR hardware emulator
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __EMULATOR_H
#define __EMULATOR_H

#ifdef __cplusplus
extern "C" {
#endif


/*------------------------------------------------------------------------------
 Standard Includes                                                                    
------------------------------------------------------------------------------*/


/*------------------------------------------------------------------------------
 Project Includes  
------------------------------------------------------------------------------*/


/*------------------------------------------------------------------------------
 Macros  
------------------------------------------------------------------------------*/


/*------------------------------------------------------------------------------
 Typedefs
------------------------------------------------------------------------------*/


/*------------------------------------------------------------------------------
 Global Variables                                             
------------------------------------------------------------------------------*/


/*------------------------------------------------------------------------------
 Exported function prototypes                                             
------------------------------------------------------------------------------*/


/*------------------------------------------------------------------------------
 Function prototypes                                             
------------------------------------------------------------------------------*/

/* firmware entry point */
int main_fut(void);

/* emulator.h */
void guisock_put
    (
    const char* message,
    size_t size
    );

/* emulator_timer.c */
void emulator_start_timers
    (
    void
    );

void emulator_buzzer_beep_request(uint32_t duration);

#ifdef __cplusplus
}
#endif

#endif /* __EMULATOR_H */


/*******************************************************************************
* END OF FILE                                                                  *
*******************************************************************************/