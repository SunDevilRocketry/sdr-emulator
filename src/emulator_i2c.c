/*******************************************************************************
*
* FILE: 
* 		emulator_i2c.c
*
* DESCRIPTION: 
* 		Mocks the functionality of I2C peripherals on the FC.
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
#include "baro.h"
#include "sdr_pin_defines_A0002.h"

/*------------------------------------------------------------------------------
 Procedure prototypes                                                       
------------------------------------------------------------------------------*/
static HAL_StatusTypeDef baro_read_handler(I2C_HandleTypeDef *hi2c, uint16_t DevAddress, uint16_t MemAddress,
                                    uint16_t MemAddSize, uint8_t *pData, uint16_t Size, uint32_t Timeout);

/*------------------------------------------------------------------------------
 HAL interfaces                                                       
------------------------------------------------------------------------------*/

HAL_StatusTypeDef HAL_I2C_Mem_Write(I2C_HandleTypeDef *hi2c, uint16_t DevAddress, uint16_t MemAddress,
                                    uint16_t MemAddSize, uint8_t *pData, uint16_t Size, uint32_t Timeout) {
    return HAL_OK;
}

HAL_StatusTypeDef HAL_I2C_Mem_Read(I2C_HandleTypeDef *hi2c, uint16_t DevAddress, uint16_t MemAddress,
                                    uint16_t MemAddSize, uint8_t *pData, uint16_t Size, uint32_t Timeout) {
if( hi2c == &( BARO_I2C ) )
    {
    return baro_read_handler( hi2c, DevAddress, MemAddress, MemAddSize, pData, Size, Timeout);
    }

    return HAL_OK;
}

HAL_StatusTypeDef HAL_I2C_Mem_Read_IT(I2C_HandleTypeDef *hi2c, uint16_t DevAddress, uint16_t MemAddress,
                                    uint16_t MemAddSize, uint8_t *pData, uint16_t Size) {
    return HAL_OK;
}

/*------------------------------------------------------------------------------
 Procedures                                                     
------------------------------------------------------------------------------*/
static HAL_StatusTypeDef baro_read_handler(I2C_HandleTypeDef *hi2c, uint16_t DevAddress, uint16_t MemAddress,
                                    uint16_t MemAddSize, uint8_t *pData, uint16_t Size, uint32_t Timeout)
{
    /* imperfect approximation, but delay a bit like the real FC */
    HAL_Delay( (uint8_t)(0.4 * Size) );

    if( MemAddress == BARO_REG_CHIP_ID )
        {
        *pData = BMP390_DEVICE_ID;
        return HAL_OK;
        }
    if( MemAddress == BARO_REG_ERR_REG )
        {
        *pData = 0x00; /* Clear the error register */
        return HAL_OK;
        }
    
    return HAL_OK;
}