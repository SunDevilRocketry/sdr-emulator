/*******************************************************************************
*
* FILE: 
* 		emulator_spi.c
*
* DESCRIPTION: 
* 		Mocks the functionality of SPI peripherals on the FC.
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
#include "flash.h"
#include "sdr_pin_defines_A0002.h"

/*------------------------------------------------------------------------------
 Statics                                                         
------------------------------------------------------------------------------*/
static bool recieve_flash_status = false; /* send flash status on next SPI_Receive */

/*------------------------------------------------------------------------------
 Procedure prototypes                                                      
------------------------------------------------------------------------------*/
static HAL_StatusTypeDef flash_spi_transmit(SPI_HandleTypeDef *hspi, const uint8_t *pData, uint16_t Size, uint32_t Timeout);
static HAL_StatusTypeDef flash_spi_receive(SPI_HandleTypeDef *hspi, uint8_t *pData, uint16_t Size, uint32_t Timeout);

/*------------------------------------------------------------------------------
 HAL interfaces                                                       
------------------------------------------------------------------------------*/

HAL_StatusTypeDef HAL_SPI_Transmit(SPI_HandleTypeDef *hspi, const uint8_t *pData, uint16_t Size, uint32_t Timeout) {
    HAL_StatusTypeDef status = HAL_OK;
    
    if( hspi == &( FLASH_SPI ) )
        {
        status = flash_spi_transmit(hspi, pData, Size, Timeout);
        }
    
    /* block for the expected amount of time rounded to nearest ms */
    float delay_time = 0.26 + (Size * 0.02);
    HAL_Delay( (uint32_t)(delay_time + 0.5) );

    return status;
}

HAL_StatusTypeDef HAL_SPI_Receive(SPI_HandleTypeDef *hspi, uint8_t *pData, uint16_t Size, uint32_t Timeout) {
    HAL_StatusTypeDef status = HAL_OK;

    if( hspi == &( FLASH_SPI ) )
        {
        status = flash_spi_receive(hspi, pData, Size, Timeout);
        }
    
    /* block for the expected amount of time rounded to nearest ms */
    float delay_time = 0.26 + (Size * 0.02);
    HAL_Delay( (uint32_t)(delay_time + 0.5) );

    return status;
}

/*------------------------------------------------------------------------------
 Procedures                                                     
------------------------------------------------------------------------------*/

static HAL_StatusTypeDef flash_spi_transmit(SPI_HandleTypeDef *hspi, const uint8_t *pData, uint16_t Size, uint32_t Timeout) 
{
/* handle flash opcode*/
if ( *pData == FLASH_OP_HW_RDSR )
    {
    recieve_flash_status = true;
    return HAL_OK;
    }

return HAL_OK;
}

static HAL_StatusTypeDef flash_spi_receive(SPI_HandleTypeDef *hspi, uint8_t *pData, uint16_t Size, uint32_t Timeout) 
{
/* handle flash opcode*/
if ( recieve_flash_status )
    {
    *pData = 0x00; /* Anything but 0xFF */
    return HAL_OK;
    }

return HAL_OK;
}