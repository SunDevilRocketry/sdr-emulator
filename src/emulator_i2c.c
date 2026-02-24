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
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>

/* POSIX */
#include <pthread.h>
#include <unistd.h>

#include "emulator.h"
#include "stm32h7xx_hal.h"
#include "baro.h"
#include "imu.h"
#include "sdr_pin_defines_A0002.h"

/*------------------------------------------------------------------------------
 Globals                                                       
------------------------------------------------------------------------------*/
extern volatile bool irq_enabled;

volatile bool imu_data_it_flag = false;
volatile bool baro_data_it_flag = false;
volatile bool mag_data_it_flag = false;

static uint8_t* imu_data_ptr;
static uint8_t* baro_data_ptr;
static uint8_t* mag_data_ptr;
static uint16_t imu_data_size = 0;
static uint16_t baro_data_size = 0;
static uint16_t mag_data_size = 0;

/* Shared synchronization objects */
static pthread_mutex_t it_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  it_cond  = PTHREAD_COND_INITIALIZER;

/*------------------------------------------------------------------------------
 Procedure prototypes                                                       
------------------------------------------------------------------------------*/
static HAL_StatusTypeDef baro_read_handler(I2C_HandleTypeDef *hi2c, uint16_t DevAddress, uint16_t MemAddress,
                                    uint16_t MemAddSize, uint8_t *pData, uint16_t Size, uint32_t Timeout);
static HAL_StatusTypeDef imu_read_handler(I2C_HandleTypeDef *hi2c, uint16_t DevAddress, uint16_t MemAddress,
                                    uint16_t MemAddSize, uint8_t *pData, uint16_t Size, uint32_t Timeout);
static HAL_StatusTypeDef mag_read_handler(I2C_HandleTypeDef *hi2c, uint16_t DevAddress, uint16_t MemAddress,
                                    uint16_t MemAddSize, uint8_t *pData, uint16_t Size, uint32_t Timeout);
static void baro_read_handler_IT();
static void imu_read_handler_IT();
static void mag_read_handler_IT();

static float sensor_add_random_noise(float readout_in, float noise_max);
static uint16_t sensor_gyro_inv(float dps);
static uint16_t sensor_acc_inv(float accel);

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
else if( ( hi2c == &( IMU_I2C ) ) 
   && ( DevAddress == IMU_ADDR ) )
    {
    return imu_read_handler( hi2c, DevAddress, MemAddress, MemAddSize, pData, Size, Timeout);
    }
else if( ( hi2c == &( IMU_I2C ) ) 
   && ( DevAddress == IMU_MAG_ADDR ) )
    {
    return mag_read_handler( hi2c, DevAddress, MemAddress, MemAddSize, pData, Size, Timeout);
    }

    return HAL_OK;
}

HAL_StatusTypeDef HAL_I2C_Mem_Read_IT(I2C_HandleTypeDef *hi2c, uint16_t DevAddress, uint16_t MemAddress,
                                    uint16_t MemAddSize, uint8_t *pData, uint16_t Size) {
if( hi2c == &( BARO_I2C ) )
    {
    pthread_mutex_lock(&it_mutex);
    baro_data_it_flag = true;
    baro_data_ptr = pData;
    baro_data_size = Size;
    pthread_cond_signal(&it_cond);
    pthread_mutex_unlock(&it_mutex);
    return HAL_OK;
    }
else if( ( hi2c == &( IMU_I2C ) ) 
   && ( DevAddress == IMU_ADDR ) )
    {
    pthread_mutex_lock(&it_mutex);
    imu_data_it_flag = true;
    imu_data_ptr = pData;
    imu_data_size = Size;
    pthread_cond_signal(&it_cond);
    pthread_mutex_unlock(&it_mutex);
    return HAL_OK;
    }
else if( ( hi2c == &( IMU_I2C ) ) 
   && ( DevAddress == IMU_MAG_ADDR ) )
    {
    pthread_mutex_lock(&it_mutex);
    mag_data_it_flag = true;
    mag_data_ptr = pData;
    mag_data_size = Size;
    pthread_cond_signal(&it_cond);
    pthread_mutex_unlock(&it_mutex);
    return HAL_OK;
    }

return HAL_OK;
}

/*------------------------------------------------------------------------------
 Procedures                                                     
------------------------------------------------------------------------------*/

/*******************************************************************************
*                                                                              *
* PROCEDURE:                                                                   * 
* 		emulator_i2c_it_listener                                               *
*                                                                              *
* DESCRIPTION:                                                                 * 
*       Listen for and fulfill i2c IT I/O.                                     *
*                                                                              *
*******************************************************************************/
void* emulator_i2c_it_listener
    (
    void* arg
    )
{
bool listening = true;
printf("Listener thread opened.\n");

while ( listening )
    {
    /* Wait until someone signals an IRQ */
    pthread_mutex_lock(&it_mutex);

    /* Wait for work - handle spurious wakeups */
    while (!imu_data_it_flag && !baro_data_it_flag && !mag_data_it_flag) {
        pthread_cond_wait(&it_cond, &it_mutex);
    }

    pthread_mutex_unlock(&it_mutex);

    /* Simulate real-time 2ms I/O delay */
    usleep(10000); /* 1000 microseconds -> 1 ms */

    /* Now process the flags */
    pthread_mutex_lock(&it_mutex); /* lock to safely access shared flags */
    bool call_imu = false, call_baro = false, call_mag = false;
        
        if (irq_enabled) {
            if (imu_data_it_flag) {
                imu_data_it_flag = false;
                call_imu = true;
            }
            if (baro_data_it_flag) {
                baro_data_it_flag = false;
                call_baro = true;
            }
            if (mag_data_it_flag) {
                mag_data_it_flag = false;
                call_mag = true;
            }
        }
        pthread_mutex_unlock(&it_mutex);
        
        /* Call handlers OUTSIDE the lock */
        if (call_imu) {
            imu_read_handler_IT();
        }
        if (call_baro) {
            baro_read_handler_IT();
        }
        if (call_mag) {
            mag_read_handler_IT();
        }
    }

    return 0;
}


/*******************************************************************************
*                                                                              *
* PROCEDURE:                                                                   * 
* 		baro_read_handler                                                      *
*                                                                              *
* DESCRIPTION:                                                                 * 
*       Handle blocking I2C reg reads for the barometer.                       *
*                                                                              *
*******************************************************************************/
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

} /* baro_read_handler */


/*******************************************************************************
*                                                                              *
* PROCEDURE:                                                                   * 
* 		imu_read_handler                                                       *
*                                                                              *
* DESCRIPTION:                                                                 * 
*       Handle blocking I2C reg reads for the IMU.                             *
*                                                                              *
*******************************************************************************/
static HAL_StatusTypeDef imu_read_handler(I2C_HandleTypeDef *hi2c, uint16_t DevAddress, uint16_t MemAddress,
                                    uint16_t MemAddSize, uint8_t *pData, uint16_t Size, uint32_t Timeout)
{
/* imperfect approximation, but delay a bit like the real FC */
HAL_Delay( (uint8_t)(0.4 * Size) );

if( MemAddress == IMU_REG_CHIP_ID )
    {
    *pData = IMU_ID;
    return HAL_OK;
    }
if( MemAddress == IMU_REG_INTERNAL_STATUS )
    {
    *pData = 0x01; /* First bit needs to be set */
    return HAL_OK;
    }

return HAL_OK;

} /* imu_read_handler */


/*******************************************************************************
*                                                                              *
* PROCEDURE:                                                                   * 
* 		mag_read_handler                                                       *
*                                                                              *
* DESCRIPTION:                                                                 * 
*       Handle blocking I2C reg reads for the magnetometer.                    *
*                                                                              *
*******************************************************************************/
static HAL_StatusTypeDef mag_read_handler(I2C_HandleTypeDef *hi2c, uint16_t DevAddress, uint16_t MemAddress,
                                    uint16_t MemAddSize, uint8_t *pData, uint16_t Size, uint32_t Timeout)
{
/* imperfect approximation, but delay a bit like the real FC */
HAL_Delay( (uint8_t)(0.4 * Size) );

if( MemAddress == MAG_REG_CHIP_ID )
    {
    *pData = MAG_ID;
    return HAL_OK;
    }
if( MemAddress == IMU_REG_INTERNAL_STATUS )
    {
    *pData = 0x01; /* First bit needs to be set */
    return HAL_OK;
    }

return HAL_OK;

} /* mag_read_handler */


/*******************************************************************************
*                                                                              *
* PROCEDURE:                                                                   * 
* 		baro_read_handler_IT                                                   *
*                                                                              *
* DESCRIPTION:                                                                 * 
*       Handle interrupt-based I2C reg reads for the barometer.                *
*                                                                              *
*******************************************************************************/
static void baro_read_handler_IT()
{
memset(baro_data_ptr, 0, baro_data_size);
baro_IT_handler();

} /* baro_read_handler_IT */


/*******************************************************************************
*                                                                              *
* PROCEDURE:                                                                   * 
* 		imu_read_handler_IT                                                    *
*                                                                              *
* DESCRIPTION:                                                                 * 
*       Handle interrupt-based I2C reg reads for the IMU.                      *
*                                                                              *
*******************************************************************************/
static void imu_read_handler_IT()
{
memset(imu_data_ptr, 0, imu_data_size);
/* accX, accY, accZ */
uint16_t accX = sensor_acc_inv(sensor_add_random_noise( 0, 0.2 ));
uint16_t accY = sensor_acc_inv(sensor_add_random_noise( 0, 0.2 ));
uint16_t accZ = sensor_acc_inv(sensor_add_random_noise( 9.8, 0.2 ));
memcpy( imu_data_ptr, &accX, 2 );
memcpy( imu_data_ptr + 2, &accY, 2 );
memcpy( imu_data_ptr + 4, &accZ, 2 );
/* gyroX, gyroY, gyroZ */
uint16_t gyroX = sensor_gyro_inv(sensor_add_random_noise( 0, 20 ));
uint16_t gyroY = sensor_gyro_inv(sensor_add_random_noise( 0, 20 ));
uint16_t gyroZ = sensor_gyro_inv(sensor_add_random_noise( 0, 20 ));
memcpy( imu_data_ptr + 6, &gyroX, 2 );
memcpy( imu_data_ptr + 8, &gyroY, 2 );
memcpy( imu_data_ptr + 10, &gyroZ, 2 );

/* Call interrupt handler */
imu_it_handler(); 

} /* imu_read_handler_IT */


/*******************************************************************************
*                                                                              *
* PROCEDURE:                                                                   * 
* 		mag_read_handler_IT                                                    *
*                                                                              *
* DESCRIPTION:                                                                 * 
*       Handle interrupt-based I2C reg reads for the magnetometer.             *
*                                                                              *
*******************************************************************************/
static void mag_read_handler_IT()
{
memset(mag_data_ptr, 0, mag_data_size);
imu_it_handler();

} /* mag_read_handler_IT */


/*******************************************************************************
*                                                                              *
* PROCEDURE:                                                                   * 
* 		sensor_add_random_noise                                                *
*                                                                              *
* DESCRIPTION:                                                                 * 
*       Adds linear noise around the given value.                              *
*                                                                              *
*******************************************************************************/
static float sensor_add_random_noise(float readout_in, float noise_max)
{
float random = rand() / (float)RAND_MAX; /* 0 - 1 */
uint8_t sign = rand() % 2;
if ( sign )
    {
    random = -random;
    }

return readout_in + (random * noise_max);

} /* sensor_add_random_noise */


/*******************************************************************************
*                                                                              *
* PROCEDURE:                                                                   * 
* 		sensor_acc_inv                                                         *
*                                                                              *
* DESCRIPTION:                                                                 * 
*       Convert a float value to the IMU accel reg format.                     *
*                                                                              *
*******************************************************************************/
static uint16_t sensor_acc_inv(float accel)
{
uint8_t g_setting = 16;
float g = 9.8f;
float accel_step = 2 * g_setting * g / 65535.0f;

/* Convert back to signed raw value */
int32_t raw = (int32_t)(accel / accel_step);

/* Clamp to int16 range (safety) */
if (raw > 32767) raw = 32767;
if (raw < -32768) raw = -32768;

/* Return as uint16 two’s complement */
return (uint16_t)((int16_t)raw);

} /* sensor_acc_inv */


/*******************************************************************************
*                                                                              *
* PROCEDURE:                                                                   * 
* 		sensor_gyro_inv                                                        *
*                                                                              *
* DESCRIPTION:                                                                 * 
*       Convert a float value to the IMU gyro reg format.                      *
*                                                                              *
*******************************************************************************/
static uint16_t sensor_gyro_inv(float dps)
{
float gyro_setting = 2000.0f;
float gyro_sens = 65535.0f / (2 * gyro_setting);

/* Convert back to signed raw */
int32_t raw = (int32_t)(dps * gyro_sens);

/* Clamp to int16 range */
if (raw > 32767) raw = 32767;
if (raw < -32768) raw = -32768;

/* Return two’s complement */
return (uint16_t)((int16_t)raw);

} /* sensor_gyro_inv */