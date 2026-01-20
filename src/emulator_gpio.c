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

#include "emulator.h"
#include "main.h"
#include "stm32h7xx_hal.h"
#include "sdr_pin_defines_A0002.h"

/*------------------------------------------------------------------------------
 HAL interfaces                                                       
------------------------------------------------------------------------------*/

void HAL_GPIO_WritePin(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin, GPIO_PinState PinState) {
    if( ( GPIOx == STATUS_GPIO_PORT ) 
     && ( GPIO_Pin & ( STATUS_G_PIN | STATUS_R_PIN | STATUS_B_PIN ) )
     && ( PinState == GPIO_PIN_RESET ) )
        {
        // ETS TEMP: Replace with IPC to GUI
        //printf("LED color changed to: ");
        switch (GPIO_Pin) 
            {
            case STATUS_G_PIN:
                //printf("GREEN\n");
                guipipe_put("LED: GREEN\n", 11);
                break;
            case STATUS_R_PIN:
                //printf("RED\n");
                guipipe_put("LED: RED\n", 9);
                break;
            case STATUS_B_PIN:
                //printf("BLUE\n");
                guipipe_put("LED: BLUE\n", 10);
                break;
            case STATUS_G_PIN | STATUS_R_PIN:
                //printf("YELLOW\n");
                guipipe_put("LED: YELLOW\n", 12);
                break;
            case STATUS_G_PIN | STATUS_B_PIN:
                //printf("CYAN\n");
                guipipe_put("LED: CYAN\n", 10);
                break;
            case STATUS_R_PIN | STATUS_B_PIN:
                //printf("PURPLE\n");
                guipipe_put("LED: PURPLE\n", 12);
                break;
            case STATUS_R_PIN | STATUS_G_PIN | STATUS_B_PIN:
                //printf("WHITE\n");
                guipipe_put("LED: WHITE\n", 11);
                break;
            default:
                printf("-=Indeterminate=-\n");
                break;
            }
        }
    if( ( GPIOx == STATUS_GPIO_PORT ) 
     && ( GPIO_Pin & ( STATUS_G_PIN | STATUS_R_PIN | STATUS_B_PIN ) )
     && ( PinState == GPIO_PIN_SET ) )
        {
        //printf("LED Reset\n");
        guipipe_put("LED: RESET\n", 11);
        }
}

GPIO_PinState HAL_GPIO_ReadPin(const GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin) {
    return GPIO_PIN_RESET;
}

/*------------------------------------------------------------------------------
 Procedures                                                     
------------------------------------------------------------------------------*/