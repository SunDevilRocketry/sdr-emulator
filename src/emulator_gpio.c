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
    if( GPIOx == STATUS_GPIO_PORT )
        {
        // ETS TEMP: Replace with IPC to GUI
        printf("LED color changed to: ");
        switch (GPIO_Pin) 
            {
            case STATUS_G_PIN:
                printf("GREEN\n");
                break;
            case STATUS_R_PIN:
                printf("RED\n");
                break;
            case STATUS_B_PIN:
                printf("BLUE\n");
                break;
            case STATUS_G_PIN | STATUS_R_PIN:
                printf("YELLOW\n");
                break;
            case STATUS_G_PIN | STATUS_B_PIN:
                printf("CYAN\n");
                break;
            case STATUS_R_PIN | STATUS_B_PIN:
                printf("PURPLE\n");
                break;
            case STATUS_R_PIN | STATUS_G_PIN | STATUS_B_PIN:
                printf("WHITE\n");
                break;
            default:
                printf("-=Indeterminate=-\n");
                break;
            }
        }
}

GPIO_PinState HAL_GPIO_ReadPin(const GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin) {
    return GPIO_PIN_RESET;
}

/*------------------------------------------------------------------------------
 Procedures                                                     
------------------------------------------------------------------------------*/