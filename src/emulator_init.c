/*******************************************************************************
*
* FILE: 
* 		emulator_init.c
*
* DESCRIPTION: 
* 		Stubs out the init routines on the FC.
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
#include "stm32h7xx_hal.h"
#include "init.h"
#include "main.h"
#include "sdr_pin_defines_A0002.h"

/*------------------------------------------------------------------------------
 Globals                                                       
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
 HAL interfaces                                                       
------------------------------------------------------------------------------*/
void BUZZER_TIM_Init() {}
void FLASH_SPI_Init() {}
void IMU_GPS_I2C_Init() {}
void Baro_I2C_Init() {}
void GPS_UART_Init() {}
void USB_UART_Init() {}
void GPIO_Init() {}
void SystemClock_Config() {}
void PeriphCommonClock_Config() {}
HAL_StatusTypeDef HAL_Init() { return HAL_OK; }

/* HTIM inits are in emulator_timer.c */

/*------------------------------------------------------------------------------
 Procedures                                                     
------------------------------------------------------------------------------*/