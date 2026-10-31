/**
 * @file emulator_flightpath.c
 *
 * Simulates a flight path by reading and interpolating a CSV. In theory the
 * sim can be restarted if you reinitialize the flightpath and start again,
 * but that isn't officially supported.
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
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>

#include "emulator.h"
#include "timer.h"

/*------------------------------------------------------------------------------
 Private Macros                                                        
------------------------------------------------------------------------------*/

#define MAX_LINE_SIZE (256) /* Longest line we could expect from the csv */
#define FLOATS_PER_LINE (14)

#define _FLIGHTPATH_EXIT_EMPTY() \
    do { emulator_log("Error: flight path file is empty or does not contain enough data.", EMULATOR_SUBSYSTEM_FLIGHTPATH); \
         emulator_exit(1); } \
    while (0)

/*------------------------------------------------------------------------------
 Private Typedefs                                                         
------------------------------------------------------------------------------*/

typedef struct _SIM_SENSOR_DATA 
    {
    float time;
    float accel_x;
    float accel_y;
    float accel_z;
    float gyro_x;
    float gyro_y;
    float gyro_z;
    float mag_x;
    float mag_y;
    float mag_z;
    float baro_pres;
    float baro_temp;
    float latitude;
    float longitude;
    } SIM_SENSOR_DATA;
_Static_assert( sizeof(SIM_SENSOR_DATA) == FLOATS_PER_LINE * sizeof(float), "Sim sensor data size invalid" );

/*------------------------------------------------------------------------------
 Static Variables                                                     
------------------------------------------------------------------------------*/

static SIM_SENSOR_DATA last_frame;
static SIM_SENSOR_DATA next_frame;

/** @brief Buffer for getting the next line of the flightpath CSV */
static char file_read_buffer[MAX_LINE_SIZE];

static FILE* flightpath_csv = NULL;

/** @brief Reference point for flightpath timing */
static double fc_arm_time = -1; 
static bool flightpath_running = false;

/*------------------------------------------------------------------------------
 Static Prototypes                                                  
------------------------------------------------------------------------------*/

static bool get_values_in_line
    (
    char* line, 
    SIM_SENSOR_DATA* data
    );

static float update_and_get_t
    (
    void
    );

static double time_helper
    (
    void
    );

/*------------------------------------------------------------------------------
 Procedures                                                     
------------------------------------------------------------------------------*/

/**
 * @brief Initializes the flightpath by loading the file.
 * 
 * @param[in] filepath Path to CSV with the simulated data.
 */
void emulator_flightpath_init
    (
    const char* filepath
    )
{
if ( flightpath_csv != NULL )
    {
    emulator_log("Warning: Stop the last flight sim before reinitalizing.", EMULATOR_SUBSYSTEM_FLIGHTPATH);
    return;
    }

flightpath_csv = fopen(filepath, "r");
emulator_log("Initializing flightpath.", EMULATOR_SUBSYSTEM_FLIGHTPATH);
if ( !flightpath_csv )
    {
    emulator_logf("Error: failed to open flight path file with errno %d.", EMULATOR_SUBSYSTEM_FLIGHTPATH, errno);
    emulator_exit(1);
    }

if ( !fgets(file_read_buffer, sizeof(file_read_buffer), flightpath_csv) ) /* skip the first line */
    {
    _FLIGHTPATH_EXIT_EMPTY();   
    }
/* Use the first line of data before arm */
if ( !fgets(file_read_buffer, sizeof(file_read_buffer), flightpath_csv)
  || !get_values_in_line(file_read_buffer, &next_frame) ) 
    {
    _FLIGHTPATH_EXIT_EMPTY();
    }
memcpy( &last_frame, &next_frame, sizeof(SIM_SENSOR_DATA) );

emulator_logf("Successfully opened flight path file %s", EMULATOR_SUBSYSTEM_FLIGHTPATH, filepath);

}

/**
 * @brief Starts the simulated flightpath.
 * 
 * @note Must be called after @ref emulator_flightpath_init.
 */
void emulator_flightpath_start
    (
    void
    )
{
if ( !flightpath_csv )
    {
    emulator_log("Error: Flightpath file was NULL.", EMULATOR_SUBSYSTEM_FLIGHTPATH);
    emulator_exit(1);
    }

if ( !flightpath_running )
    {
    emulator_log("Starting simulated flight path.", EMULATOR_SUBSYSTEM_FLIGHTPATH);
    fc_arm_time = time_helper();
    flightpath_running = true;

    rewind(flightpath_csv);

    /* Skip first line*/
    if ( !fgets(file_read_buffer, sizeof(file_read_buffer), flightpath_csv) )
        {
        _FLIGHTPATH_EXIT_EMPTY();
        }

    /* Read first frame */
    if ( !fgets(file_read_buffer, sizeof(file_read_buffer), flightpath_csv)
      || !get_values_in_line(file_read_buffer, &last_frame) )  
        {
        _FLIGHTPATH_EXIT_EMPTY();
        }
    /* Read second frame*/
    if ( !fgets(file_read_buffer, sizeof(file_read_buffer), flightpath_csv)
      || !get_values_in_line(file_read_buffer, &last_frame) )  
        {
        _FLIGHTPATH_EXIT_EMPTY();
        }
    }
}

/**
 * @brief Stops the simulated flightpath.
 */
void emulator_flightpath_stop
    (
    void
    )
{
emulator_log("Stopping simulated flight path.", EMULATOR_SUBSYSTEM_FLIGHTPATH);
flightpath_running = false;
fc_arm_time = -1;
if ( flightpath_csv != NULL )
    {
    fclose(flightpath_csv);
    flightpath_csv = NULL;
    }
}

/**
 * @brief Getter for simulated accelerometer values.
 * 
 * @param[out] ax Simulated accel X.
 * @param[out] ay Simulated accel Y.
 * @param[out] az Simulated accel Z.
 */
void emulator_flightpath_get_accel
    (
    float* ax,
    float* ay,
    float* az
    )
{
float t = update_and_get_t();

*ax = lerp(last_frame.accel_x, next_frame.accel_x, t);
*ay = lerp(last_frame.accel_y, next_frame.accel_y, t);
*az = lerp(last_frame.accel_z, next_frame.accel_z, t);
}

/**
 * @brief Getter for simulated gyroscope values.
 * 
 * @param[out] gx Simulated gyro X.
 * @param[out] gy Simulated gyro Y.
 * @param[out] gz Simulated gyro Z.
 */
void emulator_flightpath_get_gyro
    (
    float* gx,
    float* gy,
    float* gz
    )
{
float t = update_and_get_t();

*gx = lerp(last_frame.gyro_x, next_frame.gyro_x, t);
*gy = lerp(last_frame.gyro_y, next_frame.gyro_y, t);
*gz = lerp(last_frame.gyro_z, next_frame.gyro_z, t);
}

/**
 * @brief Getter for simulated magnetometer values.
 * 
 * @param[out] mag_x Simulated mag X.
 * @param[out] mag_y Simulated mag Y.
 * @param[out] mag_z Simulated mag Z.
 */
void emulator_flightpath_get_mag
    (
    float* mag_x,
    float* mag_y,
    float* mag_z
    )
{
float t = update_and_get_t();

*mag_x = lerp(last_frame.mag_x, next_frame.mag_x, t);
*mag_y = lerp(last_frame.mag_y, next_frame.mag_y, t);
*mag_z = lerp(last_frame.mag_z, next_frame.mag_z, t);
}

/**
 * @brief Getter for simulated barometer values.
 * 
 * @param[out] pres Simulated pressure.
 * @param[out] temp Simulated temperature.
 */
void emulator_flightpath_get_baro
    (
    float* pres,
    float* temp
    )
{
float t = update_and_get_t();

*pres = lerp(last_frame.baro_pres, next_frame.baro_pres, t);
*temp = lerp(last_frame.baro_temp, next_frame.baro_temp, t);
}

/**
 * @brief Updates sensor frames by advancing the line in the flightpath CSV.
 * 
 * @param time Time to update to.
 * 
 * A mutex lock might be required if this is called from multiple threads like
 * I2C and GPS.
 */
void emulator_flightpath_update
    (
    double time
    )
{
if ( !flightpath_running ) return;

float time_since_arm = (float)(time - fc_arm_time); 

/* If passed timestamp on next line, move to next line of csv */
while ( time_since_arm > next_frame.time )
    {
    memcpy(&last_frame, &next_frame, sizeof(SIM_SENSOR_DATA));
    if ( !fgets(file_read_buffer, sizeof(file_read_buffer), flightpath_csv)
      || !get_values_in_line(file_read_buffer, &next_frame) ) 
        {
        emulator_log("Reached EOF for flightpath data.", EMULATOR_SUBSYSTEM_FLIGHTPATH);
        emulator_flightpath_stop();
        return;
        }
    }

}

/**
 * @brief Calculates the proportional position of @p x between two endpoints.
 * 
 * @param x Interpolated value.
 * @param a First endpoint.
 * @param b Second endpoint.
 * 
 * @return The interpolation parameter, typically between 0 and 1. 
 */
float inverse_lerp
    (
    float x, 
    float a, 
    float b
    )
{
if (b == a) return 0.0f;
return (x - a) / (b - a);
}

/**
 * @brief Linearly interpolates between two values.
 * 
 * @param a First value.
 * @param b Second value.
 * @param t Interpolation parameter from 0 to 1.
 * 
 * @return The interpolated value. 
 */
float lerp
    (
    float a,
    float b,
    float t
    )
{
return (1.0f - t) * a + t * b;
}

/*------------------------------------------------------------------------------
 Internal Procedures                                                     
------------------------------------------------------------------------------*/

static bool get_values_in_line
    (
    char* line, 
    SIM_SENSOR_DATA* data
    ) 
{
float buffer[FLOATS_PER_LINE];

/* There won't be any commas or special characters in the strings */
char* token = strtok(line, ",");
for ( size_t i = 0; i < FLOATS_PER_LINE; i++ )
    {
    if ( token == NULL ) return false;
    buffer[i] = strtof(token, NULL);            
    token = strtok(NULL, ","); /* switch to strtok_r if multithreading */
    }

/* Yes, this is sketchy. */
memcpy(data, buffer, FLOATS_PER_LINE * sizeof(float));
return true;

}

static float update_and_get_t
    (
    void
    )
{
double time = time_helper();

emulator_flightpath_update(time);
if ( !flightpath_running ) return 1.0f;

float t = inverse_lerp( (float)(time - fc_arm_time), last_frame.time, next_frame.time );

if ( t < 0.0f ) t = 0.0f;
if ( t > 1.0f ) t = 1.0f;

return t;

}

static double time_helper
    (
    void
    )
{
struct timespec ts;
clock_gettime(CLOCK_MONOTONIC, &ts);
return (double)ts.tv_sec + (double)ts.tv_nsec * 1e-9;
}