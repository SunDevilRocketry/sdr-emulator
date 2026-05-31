/*******************************************************************************
*
* FILE: 
* 		emulator_timer.c
*
* DESCRIPTION: 
* 		Mocks out HAL timers.
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
#include <time.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

#include "emulator.h"
#include "timer.h"
#include "stm32h7xx_hal.h"
#include "sdr_pin_defines_A0002.h"

/*------------------------------------------------------------------------------
 Global Variables                                                     
------------------------------------------------------------------------------*/
uint64_t timers_start_time = 0;

/* pointers to htim instance CCRs for easy reading */
volatile uint32_t* servo_1_pulse = NULL;
volatile uint32_t* servo_2_pulse = NULL;
volatile uint32_t* servo_3_pulse = NULL;
volatile uint32_t* servo_4_pulse = NULL;

TIM_TypeDef htim2_instance;
TIM_TypeDef htim3_instance;

extern TIM_HandleTypeDef  htim2;   /* PWM 4 Timer */
extern TIM_HandleTypeDef  htim3;   /* PWM 1,2,3 Timer */

/*------------------------------------------------------------------------------
 Static Procedure Prototypes                                                   
------------------------------------------------------------------------------*/

static uint64_t get_current_time
    (
    void
    );

/*------------------------------------------------------------------------------
 HAL interfaces                                                     
------------------------------------------------------------------------------*/

void PWM123_TIM_Init() 
{
htim3.Instance = &htim3_instance;
servo_1_pulse = &(htim3.Instance->CCR4);
servo_2_pulse = &(htim3.Instance->CCR3);
servo_3_pulse = &(htim3.Instance->CCR1);
}

void PWM4_TIM_Init() 
{
htim2.Instance = &htim2_instance;
servo_4_pulse = &(htim2.Instance->CCR1);
}

/*
 * Mock function for timer driver us tick
 */
uint64_t get_us_tick
    (
    void
    )
{
return (get_current_time() - timers_start_time);
}

/*
 * Mock function for timer driver delay_ms
 */
void delay_ms
    (
    uint32_t delay
    )
{
struct timespec req = 
        {
        .tv_sec  = delay / 1000,
        .tv_nsec = (delay % 1000) * 1000000L
        };
nanosleep(&req, NULL);

} /* delay_ms */

uint32_t HAL_GetTick() {
    return (get_current_time() - timers_start_time) / 1000;
}

void HAL_Delay(uint32_t delay_time) {
    struct timespec req;
    req.tv_sec = delay_time / 1000; /* seconds */
    req.tv_nsec = (delay_time % 1000) * 1000000L; /* milliseconds to nanoseconds */
    nanosleep(&req, NULL);
}

HAL_StatusTypeDef HAL_TIM_PWM_Start
    (
    TIM_HandleTypeDef *htim, 
    uint32_t Channel
    )
{
    return HAL_OK;
}

HAL_StatusTypeDef HAL_TIM_PWM_Stop(TIM_HandleTypeDef *htim, uint32_t Channel)
{
    return HAL_OK;
}

/**
 * @brief Gets the current time since epoch (startup).
 */
SYSTEM_TIME get_system_time
    (
    void
    )
{
uint32_t systick = HAL_GetTick();
SYSTEM_TIME retval;

retval.millis = (uint16_t)(systick % MILLISEC_PER_SEC);
retval.secs = (uint8_t)((systick / MILLISEC_PER_SEC) % SEC_PER_MIN);
retval.mins = (uint8_t)((systick / (MILLISEC_PER_SEC * SEC_PER_MIN)) % MIN_PER_HOUR);
retval.hours = (uint16_t)((systick / (MILLISEC_PER_SEC * SEC_PER_MIN * MIN_PER_HOUR)));

return retval;

} /* get_system_time */

/*------------------------------------------------------------------------------
 Procedures                                                     
------------------------------------------------------------------------------*/

/*******************************************************************************
*                                                                              *
* PROCEDURE:                                                                   * 
* 		emulator_start_timers                                                  *
*                                                                              *
* DESCRIPTION:                                                                 * 
*       Initializes the timers used by the emulator.                           *
*                                                                              *
*******************************************************************************/
void emulator_start_timers
    (
    void
    )
{
timers_start_time = get_current_time();

} /* emulator_start_timers */


/*******************************************************************************
*                                                                              *
* PROCEDURE:                                                                   * 
* 		get_current_time                                                       *
*                                                                              *
* DESCRIPTION:                                                                 * 
*       Gets the current time in microseconds.                                 *
*                                                                              *
*******************************************************************************/
static uint64_t get_current_time
    (
    void
    )
{
struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts); /* monotonic never adjusts back */

    return (uint64_t)ts.tv_sec * (uint64_t)MICROSEC_PER_SEC  +
           (uint64_t)ts.tv_nsec / 1000UL;  

} /* get_current_time */


/*******************************************************************************
*                                                                              *
* PROCEDURE:                                                                   * 
* 		emulator_buzzer_beep_request                                           *
*                                                                              *
* DESCRIPTION:                                                                 * 
*       Tell the GUI to beep for the duration.                                 *
*                                                                              *
*******************************************************************************/
void emulator_buzzer_beep_request
    (
    uint32_t duration
    )
{
char dbg_msg[128];
size_t true_size = 0;
true_size = snprintf(dbg_msg, 128, "Beeping for %d ms.", duration);
emulator_debug_log(dbg_msg, true_size, EMULATOR_SUBSYSTEM_BUZZER);
HAL_Delay(duration);

} /* emulator_buzzer_beep_request */
