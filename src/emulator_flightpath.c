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
static char file_read_buffer[MAX_LINE_SIZE];

static FILE* flightpath_csv = NULL;

static float fc_arm_time = -1; /* Reference point for timing in the csv */
static bool flightpath_running = false;

/*------------------------------------------------------------------------------
 Static Prototypes                                                  
------------------------------------------------------------------------------*/

static bool get_values_in_line
    (
    char* line, 
    SIM_SENSOR_DATA* data
    );


static float time_helper
    (
    void
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

emulator_log("Successfully opened flight path file.", EMULATOR_SUBSYSTEM_FLIGHTPATH);
}

void flightpath_start
    (
    void
    )
{
if ( !flightpath_running )
    {
    emulator_log("Starting simulated flight path.", EMULATOR_SUBSYSTEM_FLIGHTPATH);
    fc_arm_time = time_helper();
    flightpath_running = true;

    rewind(flightpath_csv);
    if ( !fgets(file_read_buffer, sizeof(file_read_buffer), flightpath_csv) ) /* skip the first line */
        {
        emulator_log("Flight path file is empty", EMULATOR_SUBSYSTEM_FLIGHTPATH);
        emulator_exit(1);
        }
    }
}

void flightpath_stop
    (
    void
    )
{
emulator_log("Stopping simulated flight path.", EMULATOR_SUBSYSTEM_FLIGHTPATH);
flightpath_running = false;
fc_arm_time = -1;
fclose(flightpath_csv); // don't care about restarting flightpath so willfix later
flightpath_csv = NULL;
}


void flightpath_get_accel
    (
    float* ax,
    float* ay,
    float* az
    )
{
float time = time_helper();

// TODO: thread safety
flightpath_update(time);
float point_between = inverse_lerp(time, last_frame.time, next_frame.time);

*ax = lerp(last_frame.accel_x, next_frame.accel_x, point_between);
*ay = lerp(last_frame.accel_y, next_frame.accel_y, point_between);
*az = lerp(last_frame.accel_z, next_frame.accel_z, point_between);
}


void flightpath_get_gyro
    (
    float* gx,
    float* gy,
    float* gz
    )
{
float time = time_helper();

// TODO: thread safety
flightpath_update(time);
float point_between = inverse_lerp(time, last_frame.time, next_frame.time);

*gx = lerp(last_frame.gyro_x, next_frame.gyro_x, point_between);
*gy = lerp(last_frame.gyro_y, next_frame.gyro_y, point_between);
*gz = lerp(last_frame.gyro_z, next_frame.gyro_z, point_between);
}


void flightpath_update
    (
    float time
    )
{
if ( !flightpath_running ) return;

float time_since_arm = time - fc_arm_time; 

/* If passed timestamp on next line, move to next line of csv */
while ( time_since_arm > next_frame.time )
    {
    memcpy(&last_frame, &next_frame, sizeof(SIM_SENSOR_DATA));
    if ( !fgets(file_read_buffer, sizeof(file_read_buffer), flightpath_csv)
        || !get_values_in_line(file_read_buffer, &next_frame) ) 
        {
        emulator_log("Reached EOF for flightpath data.", EMULATOR_SUBSYSTEM_FLIGHTPATH);
        flightpath_stop();
        break;
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
    token = strtok(NULL, ",");
    }

memcpy(data, buffer, FLOATS_PER_LINE * sizeof(float));
return true;

}

static float time_helper
    (
    void
    )
{
SYSTEM_TIME systime = get_system_time();
return (float)systime.millis / MILLISEC_PER_SEC
        + (float)systime.secs
        + (float)systime.mins * SEC_PER_MIN
        + (float)systime.hours * SEC_PER_MIN * MIN_PER_HOUR;
}