/*******************************************************************************
*
* FILE: 
* 		emulator_gpio.c
*
* DESCRIPTION: 
* 		Mocks the functionality of GPIO peripherals on the FC.
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
#include <stdio.h>
#include <stdbool.h>

#include "emulator.h"
#include "main.h"
#include "stm32h7xx_hal.h"
#include "sdr_pin_defines_A0002.h"

/*------------------------------------------------------------------------------
 Globals                                                       
------------------------------------------------------------------------------*/
volatile bool ignite_flag = false;
extern int serial_port; /* DO NOT MODIFY IN THIS FILE */

/*------------------------------------------------------------------------------
 HAL interfaces                                                       
------------------------------------------------------------------------------*/

void HAL_GPIO_WritePin(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin, GPIO_PinState PinState) {
    if( ( GPIOx == STATUS_GPIO_PORT ) 
     && ( GPIO_Pin & ( STATUS_G_PIN | STATUS_R_PIN | STATUS_B_PIN ) )
     && ( PinState == GPIO_PIN_RESET ) )
        {
        setGUIStatusLED
            (
            GPIO_Pin & STATUS_R_PIN,
            GPIO_Pin & STATUS_G_PIN,
            GPIO_Pin & STATUS_B_PIN
            );
        }
        // ETS TEMP: Replace with IPC to GUI
        //printf("LED color changed to: ");
    if( ( GPIOx == STATUS_GPIO_PORT ) 
     && ( GPIO_Pin & ( STATUS_G_PIN | STATUS_R_PIN | STATUS_B_PIN ) )
     && ( PinState == GPIO_PIN_SET ) )
        {
        }
}

GPIO_PinState HAL_GPIO_ReadPin(const GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin) {
    if ( ( GPIOx == SWITCH_GPIO_PORT )
      && ( GPIO_Pin == SWITCH_PIN) )
        {
        return (GPIO_PinState)ignite_flag;
        }
    if ( ( GPIOx == USB_DETECT_GPIO_PORT )
      && ( GPIO_Pin ==  USB_DETECT_PIN) )
        {
        return ( serial_port > 0 );
        }

    return GPIO_PIN_RESET;
}

/*------------------------------------------------------------------------------
 Procedures                                                     
------------------------------------------------------------------------------*/

void setIgniteFlag
    (
    bool status
    ) 
{
ignite_flag = status;
}

