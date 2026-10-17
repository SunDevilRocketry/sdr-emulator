/**
* @file emulator_flightpath.c
*
* Simulates a flight path by reading and interpolating a csv.
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

#include "emulator.h"
#include "timer.h"

/*------------------------------------------------------------------------------
 Private Macros                                                        
------------------------------------------------------------------------------*/

#define MAX_LINE_SIZE (256) /* Longest line we could expect from the csv */
#define FLOATS_PER_LINE (14)

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
_Static_assert(sizeof(SIM_SENSOR_DATA) == FLOATS_PER_LINE * 4, "Sim sensor data size invalid");

/*------------------------------------------------------------------------------
 Static Variables                                                     
------------------------------------------------------------------------------*/

static SIM_SENSOR_DATA last_frame;
static SIM_SENSOR_DATA next_frame;

/** @brief Buffer for getting the next line of the flightpath csv */
char file_read_buffer[MAX_LINE_SIZE];

FILE* flightpath_csv;

float fc_arm_time = -1; /* Reference point for timing in the csv */

/*------------------------------------------------------------------------------
 Static Prototypes                                                  
------------------------------------------------------------------------------*/

static void get_values_in_line
    (
    char* line, 
    SIM_SENSOR_DATA* data, 
    size_t num_values
    );

static int get_exponential
    (
    const char* token
    );

static float token_to_float
    (
    const char* token
    );

/*------------------------------------------------------------------------------
 Procedures                                                     
------------------------------------------------------------------------------*/

void flightpath_init
    (
    const char* filepath
    )
{
flightpath_csv = fopen(filepath, "r");
emulator_log("Initializing flightpath.", EMULATOR_SUBSYSTEM_FLIGHTPATH);
if ( !flightpath_csv )
    {
    emulator_log("Error opening flight path file.", EMULATOR_SUBSYSTEM_FLIGHTPATH);
    emulator_exit(1);
    }

    fgets(file_read_buffer, sizeof(file_read_buffer), flightpath_csv); /* skip the first line */
    emulator_log("Successfully opened flight path file.", EMULATOR_SUBSYSTEM_FLIGHTPATH);
}

void flightpath_start
    (
    void
    )
{
fc_arm_time = (float)HAL_GetTick() / (float)MILLISEC_PER_SEC;
}

void flightpath_stop
    (
    void
    )
{
fc_arm_time = -INFINITY;
}

// can get time in here, but should we?
// this is really dumb as is right now
void flightpath_get_accel
    (
    float* ax,
    float* ay,
    float* az
    )
{
float time = (float)HAL_GetTick() / (float)MILLISEC_PER_SEC; /* Kinda hacky but whatever */
flightpath_update(time);
float point_between = inverse_lerp(time, last_frame.time, next_frame.time);

*ax = lerp(last_frame.accel_x, next_frame.accel_x, point_between);
*ay = lerp(last_frame.accel_y, next_frame.accel_y, point_between);
*az = lerp(last_frame.accel_z, next_frame.accel_z, point_between);
}


// needs to get called fairly often
void flightpath_update
    (
    float time
    )
{
    if ( fc_arm_time < 0 ) return;
    float time_since_arm = time - fc_arm_time; 

    /* If passed timestamp on next line, move to next line of csv */
    if ( time_since_arm > next_frame.time )
    {
        memcpy(&last_frame, &next_frame, sizeof(SIM_SENSOR_DATA)); // has the problem that we might skip a line
        if ( fgets(file_read_buffer, sizeof(file_read_buffer), flightpath_csv) != NULL ) 
            {
            get_values_in_line(file_read_buffer, &next_frame, FLOATS_PER_LINE);
            }
        else
            {
            emulator_log("Reached EOF for flightpath data.", EMULATOR_SUBSYSTEM_FLIGHTPATH);
            flightpath_stop();
            }
    }
}


float inverse_lerp
    (
    float x, 
    float x1, 
    float x2
    )
{
if (x2 == x1) return 0.0f;
return (x - x1) / (x2 - x1);
}

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

static int get_exponential
    (
    const char* token
    ) 
{
int exponent = 0;

char* e_ptr = strchr(token, 'e'); /* No uppercase E */
if ( e_ptr != NULL )    
    {
    exponent = atoi(e_ptr + 1);
    }

return exponent;
}

static float token_to_float
    (
    const char* token
    )
{
int exponent = get_exponential(token);
float mantissa = strtof(token, NULL);

return mantissa * powf(10, exponent);
}

static void get_values_in_line
    (
    char* line, 
    SIM_SENSOR_DATA* data, 
    size_t num_values
    ) 
{
float buffer[num_values];

/* There won't be any commas or special characters in the strings */
char* token = strtok(line, ",");
for ( size_t i = 0; i < num_values; i++ )
    {
    buffer[i] = token_to_float(token);            
    token = strtok(NULL, ",");
    }

memcpy(data, buffer, FLOATS_PER_LINE * sizeof(float));
}