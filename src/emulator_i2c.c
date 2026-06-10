/**
* @file emulator_i2c.c
*
* Mocks the functionality of I2C peripherals on the FC.
*                                                                             
* @copyright                                                                  
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
*/

/*------------------------------------------------------------------------------
 Includes                                                         
------------------------------------------------------------------------------*/
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <math.h>

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

/* NVM / trim data served to driver init via blocking I2C reads */
static uint8_t emulator_baro_cal_buffer[BARO_CAL_BUFFER_SIZE];
static BARO_CAL_DATA emulator_baro_cal;
static MAG_TRIM emulator_mag_trim_preset;

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
static int16_t sensor_mag_xy_inv(float ut);
static int16_t sensor_mag_z_inv(float ut);
static uint32_t sensor_baro_temp_inv(float temp_c);
static uint32_t sensor_baro_pres_inv(float pres_pa, float temp_c);
static float emulator_temp_compensate(uint32_t raw_readout);
static float emulator_press_compensate(uint32_t raw_readout);
static void emulator_apply_baro_cal_buffer(void);
static uint8_t emulator_mag_trim_reg_read(uint8_t reg_addr);
static void emulator_i2c_cal_init(void);
static void sensor_baro_raw_store(uint32_t raw, uint8_t *bytes);
static void sensor_mag_xy_pack(int16_t raw, uint8_t *lsb, uint8_t *msb);
static void sensor_mag_z_pack(int16_t raw, uint8_t *lsb, uint8_t *msb);
static void sensor_mag_rhall_pack(uint16_t rhall, uint8_t *lsb, uint8_t *msb);

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
emulator_i2c_cal_init();
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
        
        if ( emulator_flags_check_bits(IRQ_ENABLED_FLAG_BIT) ) {
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
if( MemAddress >= BARO_REG_NVM_PAR_T1 &&
    ( MemAddress + Size ) <= ( BARO_REG_NVM_PAR_T1 + BARO_CAL_BUFFER_SIZE ) )
    {
    memcpy( pData,
            &emulator_baro_cal_buffer[MemAddress - BARO_REG_NVM_PAR_T1],
            Size );
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
if( MemAddress >= MAG_TRIM_REG_X1 && MemAddress <= MAG_TRIM_REG_XY1 )
    {
    for( uint16_t i = 0; i < Size; i++ )
        {
        pData[i] = emulator_mag_trim_reg_read( (uint8_t)( MemAddress + i ) );
        }
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
float pres_pa;
float temp_c;
uint32_t raw_pres;
uint32_t raw_temp;

memset(baro_data_ptr, 0, baro_data_size);

/* 97.077 kPa and 32 degC with sensor noise (driver/baro IT buffer layout) */
pres_pa = sensor_add_random_noise( 97077.0f, 5.0f );
temp_c  = sensor_add_random_noise( 32.0f, 0.1f );
raw_temp = sensor_baro_temp_inv( temp_c );
raw_pres = sensor_baro_pres_inv( pres_pa, temp_c );
sensor_baro_raw_store( raw_pres, baro_data_ptr );
sensor_baro_raw_store( raw_temp, baro_data_ptr + 3 );

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
uint16_t accX = sensor_acc_inv(sensor_add_random_noise( 9.8, 0.2 ));
uint16_t accY = sensor_acc_inv(sensor_add_random_noise( 0, 0.2 ));
uint16_t accZ = sensor_acc_inv(sensor_add_random_noise( 0, 0.2 ));
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
int16_t mag_x_raw;
int16_t mag_y_raw;
int16_t mag_z_raw;
uint16_t rhall;
MAG_TRIM trim;

memset(mag_data_ptr, 0, mag_data_size);

mag_x_raw = sensor_mag_xy_inv( sensor_add_random_noise( -1.3f, 0.2f ) );
mag_y_raw = sensor_mag_xy_inv( sensor_add_random_noise( 1.9f, 0.2f ) );
mag_z_raw = sensor_mag_z_inv( sensor_add_random_noise( -60.0f, 2.0f ) );
sensor_mag_xy_pack( mag_x_raw, mag_data_ptr, mag_data_ptr + 1 );
sensor_mag_xy_pack( mag_y_raw, mag_data_ptr + 2, mag_data_ptr + 3 );
sensor_mag_z_pack( mag_z_raw, mag_data_ptr + 4, mag_data_ptr + 5 );

trim = imu_get_mag_trim();
rhall = trim.dig_xyz1;
sensor_mag_rhall_pack( rhall, mag_data_ptr + 6, mag_data_ptr + 7 );

/* call interrupt handler */
imu_it_handler();

} /* mag_read_handler_IT */


/*------------------------------------------------------------------------------
 Helpers                                                     
------------------------------------------------------------------------------*/

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
* 		sensor_baro_raw_store                                                  *
*                                                                              *
* DESCRIPTION:                                                                 *
*       Store a 24-bit BMP390 ADC value into three registers (LSB first).      *
*                                                                              *
*******************************************************************************/
static void sensor_baro_raw_store(uint32_t raw, uint8_t *bytes)
{
bytes[0] = (uint8_t)( raw & 0xFF );
bytes[1] = (uint8_t)( ( raw >> 8 ) & 0xFF );
bytes[2] = (uint8_t)( ( raw >> 16 ) & 0xFF );

} /* sensor_baro_raw_store */


/*******************************************************************************
*                                                                              *
* PROCEDURE:                                                                   *
* 		sensor_mag_xy_pack                                                     *
*                                                                              *
* DESCRIPTION:                                                                 *
*       Pack 13-bit XY mag ADC into BMM150 register bytes (driver/imu/imu.c).  *
*                                                                              *
*******************************************************************************/
static void sensor_mag_xy_pack(int16_t raw, uint8_t *lsb, uint8_t *msb)
{
uint16_t val = (uint16_t)raw & 0x1FFF;

*msb = (uint8_t)( ( val >> MAG_XY_MSB_BITSHIFT ) & 0xFF );
*lsb = (uint8_t)( ( ( val & 0x1F ) << MAG_XY_LSB_BITSHIFT ) & MAG_XY_LSB_BITMASK );

} /* sensor_mag_xy_pack */


/*******************************************************************************
*                                                                              *
* PROCEDURE:                                                                   *
* 		sensor_mag_z_pack                                                      *
*                                                                              *
* DESCRIPTION:                                                                 *
*       Pack 15-bit Z mag ADC into BMM150 register bytes (driver/imu/imu.c).   *
*                                                                              *
*******************************************************************************/
static void sensor_mag_z_pack(int16_t raw, uint8_t *lsb, uint8_t *msb)
{
uint16_t val = (uint16_t)(int16_t)raw;

if (raw > 16383)  { raw = 16383;  val = (uint16_t)raw; }
if (raw < -16384) { raw = -16384; val = (uint16_t)raw; }

*msb = (uint8_t)( ( val >> MAG_Z_MSB_BITSHIFT ) & 0xFF );
*lsb = (uint8_t)( ( ( val & 0x7F ) << MAG_Z_LSB_BITSHIFT ) & MAG_Z_LSB_BITMASK );

} /* sensor_mag_z_pack */


/*******************************************************************************
*                                                                              *
* PROCEDURE:                                                                   *
* 		sensor_mag_rhall_pack                                                  *
*                                                                              *
* DESCRIPTION:                                                                 *
*       Pack hall resistance into BMM150 register bytes (driver/imu/imu.c).  *
*                                                                              *
*******************************************************************************/
static void sensor_mag_rhall_pack(uint16_t rhall, uint8_t *lsb, uint8_t *msb)
{
*msb = (uint8_t)( ( rhall >> MAG_RHALL_MSB_BITSHIFT ) & 0xFF );
*lsb = (uint8_t)( ( ( rhall & 0x3F ) << MAG_RHALL_LSB_BITSHIFT ) & MAG_RHALL_LSB_BITMASK );

} /* sensor_mag_rhall_pack */


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


/*******************************************************************************
*                                                                              *
* PROCEDURE:                                                                   *
* 		sensor_mag_xy_inv                                                      *
*                                                                              *
* DESCRIPTION:                                                                 *
*       Convert a float value to BMM150 XY magnetometer raw (13-bit) format.   *
*       Inverse of sensor_conv_mag XY scaling (mod/sensor/sensor.c).             *
*                                                                              *
*******************************************************************************/
static int16_t sensor_mag_xy_inv(float ut)
{
MAG_TRIM trim = imu_get_mag_trim();
float process_comp_x4 = ((float)trim.dig_x2) + 160.0f;
float mag_sens = process_comp_x4 / 5120.0f;

int32_t raw = (int32_t)(ut / mag_sens);

if (raw > 4095) raw = 4095;
if (raw < -4096) raw = -4096;

return (int16_t)raw;

} /* sensor_mag_xy_inv */


/*******************************************************************************
*                                                                              *
* PROCEDURE:                                                                   *
* 		sensor_mag_z_inv                                                       *
*                                                                              *
* DESCRIPTION:                                                                 *
*       Convert a float value to BMM150 Z magnetometer raw (15-bit) format.    *
*       Inverse of sensor_conv_mag Z scaling (mod/sensor/sensor.c).            *
*                                                                              *
*******************************************************************************/
static int16_t sensor_mag_z_inv(float ut)
{
MAG_TRIM trim = imu_get_mag_trim();
float mag_sens;

if (trim.dig_z2 == 0)
    {
    mag_sens = 40.0f;
    }
else
    {
    mag_sens = 40.0f * ((float)trim.dig_z2) / ((float)trim.dig_z1);
    }

int32_t raw = (int32_t)(ut * mag_sens + ((float)trim.dig_z4) * 128.0f);

if (raw > 32767) raw = 32767;
if (raw < -32768) raw = -32768;

return (int16_t)raw;

} /* sensor_mag_z_inv */


/*******************************************************************************
*                                                                              *
* PROCEDURE:                                                                   *
* 		emulator_temp_compensate                                               *
*                                                                              *
* DESCRIPTION:                                                                 *
*       BMP390 temperature compensation (driver/baro/baro.c temp_compensate).  *
*                                                                              *
*******************************************************************************/
static float emulator_temp_compensate(uint32_t raw_readout)
{
float partial_data1;
float partial_data2;

partial_data1 = (float)(raw_readout - emulator_baro_cal.par_t1);
partial_data2 = (float)(partial_data1 * emulator_baro_cal.par_t2);
emulator_baro_cal.comp_temp = (float)(partial_data2 +
                            powf(partial_data1, 2) * emulator_baro_cal.par_t3);
return emulator_baro_cal.comp_temp;

} /* emulator_temp_compensate */


/*******************************************************************************
*                                                                              *
* PROCEDURE:                                                                   *
* 		emulator_press_compensate                                              *
*                                                                              *
* DESCRIPTION:                                                                 *
*       BMP390 pressure compensation (driver/baro/baro.c press_compensate).    *
*                                                                              *
*******************************************************************************/
static float emulator_press_compensate(uint32_t raw_readout)
{
float partial_data1;
float partial_data2;
float partial_data3;
float partial_data4;
float partial_out1;
float partial_out2;

partial_data1 = emulator_baro_cal.par_p6 * emulator_baro_cal.comp_temp;
partial_data2 = emulator_baro_cal.par_p7 * powf(emulator_baro_cal.comp_temp, 2);
partial_data3 = emulator_baro_cal.par_p8 * powf(emulator_baro_cal.comp_temp, 3);
partial_out1  = (emulator_baro_cal.par_p5 + partial_data1 +
                 partial_data2 + partial_data3);

partial_data1 = emulator_baro_cal.par_p2 * emulator_baro_cal.comp_temp;
partial_data2 = emulator_baro_cal.par_p3 * powf(emulator_baro_cal.comp_temp, 2);
partial_data3 = emulator_baro_cal.par_p4 * powf(emulator_baro_cal.comp_temp, 3);
partial_out2  = (float)raw_readout * (emulator_baro_cal.par_p1 +
                                        partial_data1 +
                                        partial_data2 +
                                        partial_data3);

partial_data1 = powf((float)raw_readout, 2);
partial_data2 = (emulator_baro_cal.par_p9 +
                 emulator_baro_cal.par_p10 * emulator_baro_cal.comp_temp);
partial_data3 = partial_data1 * partial_data2;
partial_data4 = (partial_data3 +
                 powf((float)raw_readout, 3) * emulator_baro_cal.par_p11);

return partial_out1 + partial_out2 + partial_data4;

} /* emulator_press_compensate */


/*******************************************************************************
*                                                                              *
* PROCEDURE:                                                                   *
* 		sensor_baro_temp_inv                                                   *
*                                                                              *
* DESCRIPTION:                                                                 *
*       Convert a temperature (deg C) to BMP390 raw register format.           *
*       Inverse of temp_compensate (driver/baro/baro.c).                       *
*                                                                              *
*******************************************************************************/
static uint32_t sensor_baro_temp_inv(float temp_c)
{
float partial_data1;
float discriminant;
float x;

if (emulator_baro_cal.par_t3 != 0.0f)
    {
    discriminant = (emulator_baro_cal.par_t2 * emulator_baro_cal.par_t2) +
                   (4.0f * emulator_baro_cal.par_t3 * temp_c);
    if (discriminant < 0.0f)
        {
        discriminant = 0.0f;
        }
    x = (-emulator_baro_cal.par_t2 + sqrtf(discriminant)) / (2.0f * emulator_baro_cal.par_t3);
    }
else if (emulator_baro_cal.par_t2 != 0.0f)
    {
    x = temp_c / emulator_baro_cal.par_t2;
    }
else
    {
    x = 0.0f;
    }

partial_data1 = x + emulator_baro_cal.par_t1;

if (partial_data1 < 0.0f) partial_data1 = 0.0f;
if (partial_data1 > 16777215.0f) partial_data1 = 16777215.0f;

return (uint32_t)partial_data1;

} /* sensor_baro_temp_inv */


/*******************************************************************************
*                                                                              *
* PROCEDURE:                                                                   *
* 		sensor_baro_pres_inv                                                   *
*                                                                              *
* DESCRIPTION:                                                                 *
*       Convert a pressure (Pa) to BMP390 raw register format.                 *
*       Inverse of press_compensate (driver/baro/baro.c).                      *
*                                                                              *
*******************************************************************************/
static uint32_t sensor_baro_pres_inv(float pres_pa, float temp_c)
{
uint32_t raw_lo = 0;
uint32_t raw_hi = 0xFFFFFF;
uint32_t raw_mid;
uint32_t raw_temp;
float comp_pres;
float comp_at_lo;
float comp_at_hi;

raw_temp = sensor_baro_temp_inv(temp_c);
emulator_temp_compensate(raw_temp);

comp_at_lo = emulator_press_compensate( raw_lo );
comp_at_hi = emulator_press_compensate( raw_hi );

/* BMP390 comp pressure vs raw ADC can decrease with these coeffs */
while (raw_lo < raw_hi)
    {
    raw_mid = raw_lo + ((raw_hi - raw_lo) / 2);
    comp_pres = emulator_press_compensate(raw_mid);

    if (comp_at_lo > comp_at_hi)
        {
        if (comp_pres > pres_pa)
            {
            raw_lo = raw_mid + 1;
            }
        else
            {
            raw_hi = raw_mid;
            }
        }
    else
        {
        if (comp_pres < pres_pa)
            {
            raw_lo = raw_mid + 1;
            }
        else
            {
            raw_hi = raw_mid;
            }
        }
    }

return raw_lo;

} /* sensor_baro_pres_inv */


static uint16_t emulator_bytes_to_uint16_t(uint8_t lsb_byte, uint8_t msb_byte)
{
return ( ( (uint16_t) lsb_byte      ) |
         ( ( (uint16_t) msb_byte << 8 ) ) );
}

static int16_t emulator_bytes_to_int16_t(uint8_t lsb_byte, uint8_t msb_byte)
{
uint16_t bytes_comb = emulator_bytes_to_uint16_t( lsb_byte, msb_byte );
return (int16_t) bytes_comb;
}

/*******************************************************************************
*                                                                              *
* PROCEDURE:                                                                   *
* 		emulator_apply_baro_cal_buffer                                         *
*                                                                              *
* DESCRIPTION:                                                                 *
*       Parse emulator NVM bytes into float coeffs (driver/baro/baro.c).       *
*                                                                              *
*******************************************************************************/
static void emulator_apply_baro_cal_buffer(void)
{
BARO_CAL_DATA_INT cal_data_int;

cal_data_int.par_t1  = emulator_bytes_to_uint16_t( emulator_baro_cal_buffer[0],
                                                   emulator_baro_cal_buffer[1] );
cal_data_int.par_t2  = emulator_bytes_to_uint16_t( emulator_baro_cal_buffer[2],
                                                   emulator_baro_cal_buffer[3] );
cal_data_int.par_t3  = (int8_t) emulator_baro_cal_buffer[4];
cal_data_int.par_p1  = emulator_bytes_to_int16_t( emulator_baro_cal_buffer[5],
                                                  emulator_baro_cal_buffer[6] );
cal_data_int.par_p2  = emulator_bytes_to_int16_t( emulator_baro_cal_buffer[7],
                                                  emulator_baro_cal_buffer[8] );
cal_data_int.par_p3  = (int8_t) emulator_baro_cal_buffer[9];
cal_data_int.par_p4  = (int8_t) emulator_baro_cal_buffer[10];
cal_data_int.par_p5  = emulator_bytes_to_uint16_t( emulator_baro_cal_buffer[11],
                                                   emulator_baro_cal_buffer[12] );
cal_data_int.par_p6  = emulator_bytes_to_uint16_t( emulator_baro_cal_buffer[13],
                                                   emulator_baro_cal_buffer[14] );
cal_data_int.par_p7  = (int8_t) emulator_baro_cal_buffer[15];
cal_data_int.par_p8  = (int8_t) emulator_baro_cal_buffer[16];
cal_data_int.par_p9  = emulator_bytes_to_int16_t( emulator_baro_cal_buffer[17],
                                                  emulator_baro_cal_buffer[18] );
cal_data_int.par_p10 = (int8_t) emulator_baro_cal_buffer[19];
cal_data_int.par_p11 = (int8_t) emulator_baro_cal_buffer[20];

emulator_baro_cal.par_t1 = ( (float) cal_data_int.par_t1 ) / 0.00390625f;
emulator_baro_cal.par_t2 = ( (float) cal_data_int.par_t2 ) / 1073741824.0f;
emulator_baro_cal.par_t3 = ( (float) cal_data_int.par_t3 ) / 281474976710656.0f;
emulator_baro_cal.par_p1 = ( (float) ( cal_data_int.par_p1 - 16384 ) ) / 1048576.0f;
emulator_baro_cal.par_p2 = ( (float) ( cal_data_int.par_p2 - 16384 ) ) / 536870912.0f;
emulator_baro_cal.par_p3 = ( (float) cal_data_int.par_p3 ) / 4294967296.0f;
emulator_baro_cal.par_p4 = ( (float) cal_data_int.par_p4 ) / 137438953472.0f;
emulator_baro_cal.par_p5 = ( (float) cal_data_int.par_p5 ) / 0.125f;
emulator_baro_cal.par_p6 = ( (float) cal_data_int.par_p6 ) / 64.0f;
emulator_baro_cal.par_p7 = ( (float) cal_data_int.par_p7 ) / 256.0f;
emulator_baro_cal.par_p8 = ( (float) cal_data_int.par_p8 ) / 32768.0f;
emulator_baro_cal.par_p9 = ( (float) cal_data_int.par_p9 ) / 281474976710656.0f;
emulator_baro_cal.par_p10 = ( (float) cal_data_int.par_p10 ) / 281474976710656.0f;
emulator_baro_cal.par_p11 = ( (float) cal_data_int.par_p11 ) / 36893488147419103232.0f;
emulator_baro_cal.comp_temp = 0.0f;

} /* emulator_apply_baro_cal_buffer */


/*******************************************************************************
*                                                                              *
* PROCEDURE:                                                                   *
* 		emulator_mag_trim_reg_read                                           *
*                                                                              *
* DESCRIPTION:                                                                 *
*       Return magnetometer trim register bytes (driver/imu/imu.c mag_init).   *
*                                                                              *
*******************************************************************************/
static uint8_t emulator_mag_trim_reg_read(uint8_t reg_addr)
{
switch( reg_addr )
    {
    case MAG_TRIM_REG_X1:
        return (uint8_t) emulator_mag_trim_preset.dig_x1;
    case MAG_TRIM_REG_Y1:
        return (uint8_t) emulator_mag_trim_preset.dig_y1;
    case MAG_TRIM_REG_Z4_LSB:
        return (uint8_t)( emulator_mag_trim_preset.dig_z4 & 0xFF );
    case MAG_TRIM_REG_Z4_MSB:
        return (uint8_t)( ( emulator_mag_trim_preset.dig_z4 >> 8 ) & 0xFF );
    case MAG_TRIM_REG_X2:
        return (uint8_t) emulator_mag_trim_preset.dig_x2;
    case MAG_TRIM_REG_Y2:
        return (uint8_t) emulator_mag_trim_preset.dig_y2;
    case MAG_TRIM_REG_Z2_LSB:
        return (uint8_t)( emulator_mag_trim_preset.dig_z2 & 0xFF );
    case MAG_TRIM_REG_Z2_MSB:
        return (uint8_t)( ( emulator_mag_trim_preset.dig_z2 >> 8 ) & 0xFF );
    case MAG_TRIM_REG_Z1_LSB:
        return (uint8_t)( emulator_mag_trim_preset.dig_z1 & 0xFF );
    case MAG_TRIM_REG_Z1_MSB:
        return (uint8_t)( ( emulator_mag_trim_preset.dig_z1 >> 8 ) & 0xFF );
    case MAG_TRIM_REG_XYZ1_LSB:
        return (uint8_t)( emulator_mag_trim_preset.dig_xyz1 & 0xFF );
    case MAG_TRIM_REG_XYZ1_MSB:
        return (uint8_t)( ( emulator_mag_trim_preset.dig_xyz1 >> 8 ) & 0x7F );
    case MAG_TRIM_REG_Z3_LSB:
        return (uint8_t)( emulator_mag_trim_preset.dig_z3 & 0xFF );
    case MAG_TRIM_REG_Z3_MSB:
        return (uint8_t)( ( emulator_mag_trim_preset.dig_z3 >> 8 ) & 0xFF );
    case MAG_TRIM_REG_XY2:
        return (uint8_t) emulator_mag_trim_preset.dig_xy2;
    case MAG_TRIM_REG_XY1:
        return emulator_mag_trim_preset.dig_xy1;
    default:
        return 0;
    }
} /* emulator_mag_trim_reg_read */


/*******************************************************************************
*                                                                              *
* PROCEDURE:                                                                   *
* 		emulator_i2c_cal_init                                                  *
*                                                                              *
* DESCRIPTION:                                                                 *
*       Load baro NVM and mag trim values served on blocking I2C reads.       *
*                                                                              *
*******************************************************************************/
static void emulator_i2c_cal_init(void)
{
BARO_CAL_DATA_INT cal;
uint16_t temp_u16;
int16_t temp_i16;

memset( &cal, 0, sizeof( cal ) );

/* Pressure coeffs from driver/baro/comp.py (representative BMP390) */
cal.par_p1  = (int16_t)( -0.00868797302f * 1048576.0f + 16384.0f );
cal.par_p2  = (int16_t)( -1.95223838e-05f * 536870912.0f + 16384.0f );
cal.par_p3  = (int8_t)( 1.39698386e-09f * 4294967296.0f );
cal.par_p4  = (int8_t)( 7.27595761e-12f * 137438953472.0f );
cal.par_p5  = (uint16_t)( 154288.0f * 0.125f );
cal.par_p6  = (uint16_t)( 365.09375f * 64.0f );
cal.par_p7  = (int8_t)( 0.01171875f * 256.0f );
cal.par_p8  = (int8_t)( -0.000183105469f * 32768.0f );
cal.par_p9  = (int16_t)( 1.40367717e-11f * 281474976710656.0f );
cal.par_p10 = (int8_t)( 2.13162821e-14f * 281474976710656.0f );
cal.par_p11 = (int8_t)( -2.98155597e-19f * 36893488147419103232.0f );

/* Temperature coeffs tuned for ~25 degC at mid-scale raw ADC */
cal.par_t1 = 0;
cal.par_t2 = (uint16_t)( ( 25.0f / 8388608.0f ) * 1073741824.0f );
cal.par_t3 = 0;

emulator_baro_cal_buffer[0]  = (uint8_t)( cal.par_t1 & 0xFF );
emulator_baro_cal_buffer[1]  = (uint8_t)( ( cal.par_t1 >> 8 ) & 0xFF );
emulator_baro_cal_buffer[2]  = (uint8_t)( cal.par_t2 & 0xFF );
emulator_baro_cal_buffer[3]  = (uint8_t)( ( cal.par_t2 >> 8 ) & 0xFF );
emulator_baro_cal_buffer[4]  = (uint8_t) cal.par_t3;
temp_i16 = cal.par_p1;
emulator_baro_cal_buffer[5]  = (uint8_t)( temp_i16 & 0xFF );
emulator_baro_cal_buffer[6]  = (uint8_t)( ( temp_i16 >> 8 ) & 0xFF );
temp_i16 = cal.par_p2;
emulator_baro_cal_buffer[7]  = (uint8_t)( temp_i16 & 0xFF );
emulator_baro_cal_buffer[8]  = (uint8_t)( ( temp_i16 >> 8 ) & 0xFF );
emulator_baro_cal_buffer[9]  = (uint8_t) cal.par_p3;
emulator_baro_cal_buffer[10] = (uint8_t) cal.par_p4;
temp_u16 = cal.par_p5;
emulator_baro_cal_buffer[11] = (uint8_t)( temp_u16 & 0xFF );
emulator_baro_cal_buffer[12] = (uint8_t)( ( temp_u16 >> 8 ) & 0xFF );
temp_u16 = cal.par_p6;
emulator_baro_cal_buffer[13] = (uint8_t)( temp_u16 & 0xFF );
emulator_baro_cal_buffer[14] = (uint8_t)( ( temp_u16 >> 8 ) & 0xFF );
emulator_baro_cal_buffer[15] = (uint8_t) cal.par_p7;
emulator_baro_cal_buffer[16] = (uint8_t) cal.par_p8;
temp_i16 = cal.par_p9;
emulator_baro_cal_buffer[17] = (uint8_t)( temp_i16 & 0xFF );
emulator_baro_cal_buffer[18] = (uint8_t)( ( temp_i16 >> 8 ) & 0xFF );
emulator_baro_cal_buffer[19] = (uint8_t) cal.par_p10;
emulator_baro_cal_buffer[20] = (uint8_t) cal.par_p11;

emulator_apply_baro_cal_buffer();

/* BMM150 trim preset (regular mode, non-zero for mag_inv scaling) */
emulator_mag_trim_preset.dig_x1  = 66;
emulator_mag_trim_preset.dig_y1  = 77;
emulator_mag_trim_preset.dig_x2  = 1;
emulator_mag_trim_preset.dig_y2  = 1;
emulator_mag_trim_preset.dig_z1  = 762;
emulator_mag_trim_preset.dig_z2  = 660;
emulator_mag_trim_preset.dig_z3  = 0;
emulator_mag_trim_preset.dig_z4  = 0;
emulator_mag_trim_preset.dig_xy1 = 38;
emulator_mag_trim_preset.dig_xy2 = 1;
emulator_mag_trim_preset.dig_xyz1 = 450;

} /* emulator_i2c_cal_init */
