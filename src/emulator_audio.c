/*******************************************************************************
*
* FILE: 
* 		emulator_audio.c
*
* DESCRIPTION: 
* 		Implements a cross-platform audio device for the emulator's buzzer.
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
#include "stm32h7xx_hal.h"

/* External libs */
#include "miniaudio.h"

/*------------------------------------------------------------------------------
 Defines                                                         
------------------------------------------------------------------------------*/
#define PIEZO_FREQ 3800
#define BUZZ_VOL 1

/* Audio format */
#define DEVICE_FORMAT       ma_format_f32
#define DEVICE_CHANNELS     1 /* mono */
#define DEVICE_SAMPLE_RATE  48000 /* 48 kHz sample rate */

/*------------------------------------------------------------------------------
 Global Variables                                                     
------------------------------------------------------------------------------*/

/* Audio playback */
static bool buzzer_on = false;
static ma_device device;
static ma_waveform sawWave;

/*------------------------------------------------------------------------------
 Static Procedure Prototypes                                                   
------------------------------------------------------------------------------*/

static void buzzer_wave_callback
    (
    ma_device* pDevice, 
    void* pOutput, 
    const void* pInput, 
    ma_uint32 frameCount
    );

/*------------------------------------------------------------------------------
 Procedures                                                     
------------------------------------------------------------------------------*/

/*******************************************************************************
*                                                                              *
* PROCEDURE:                                                                   * 
* 		emulator_buzzer_init                                                   *
*                                                                              *
* DESCRIPTION:                                                                 * 
*       Initialize the audio library used for the buzzer.                      *
*                                                                              *
*******************************************************************************/
void emulator_buzzer_init
    (
    void
    )
{
ma_device_config deviceConfig;
ma_waveform_config sawWaveConfig;

deviceConfig = ma_device_config_init(ma_device_type_playback);
deviceConfig.playback.format   = DEVICE_FORMAT;
deviceConfig.playback.channels = DEVICE_CHANNELS;
deviceConfig.sampleRate        = DEVICE_SAMPLE_RATE;
deviceConfig.dataCallback      = buzzer_wave_callback;
deviceConfig.pUserData         = &sawWave;

if (ma_device_init(NULL, &deviceConfig, &device) != MA_SUCCESS) {
    printf("Emulator Init: Failed to open playback device.\n");
    emulator_exit(-1);
}

printf("Device Name: %s\n", device.playback.name);

sawWaveConfig = ma_waveform_config_init(device.playback.format, device.playback.channels, device.sampleRate, ma_waveform_type_sawtooth, BUZZ_VOL, PIEZO_FREQ);
ma_waveform_init(&sawWaveConfig, &sawWave);

} /* emulator_buzzer_init */


/*******************************************************************************
*                                                                              *
* PROCEDURE:                                                                   * 
* 		emulator_buzzer_teardown                                               *
*                                                                              *
* DESCRIPTION:                                                                 * 
*       De-initializes the emulator buzzer.                                    *
*                                                                              *
*******************************************************************************/
void emulator_buzzer_teardown
    (
    void
    )
{
ma_device_uninit(&device);

} /* emulator_buzzer_teardown */


/*******************************************************************************
*                                                                              *
* PROCEDURE:                                                                   * 
* 		emulator_buzzer_beep_request                                           *
*                                                                              *
* DESCRIPTION:                                                                 * 
*       Tell the GUI to beep for the duration (blocking).                      *
*                                                                              *
*******************************************************************************/
void emulator_buzzer_beep_request
    (
    uint32_t duration
    )
{
/* Local Variables */
buzzer_on = true;
uint64_t start_time = HAL_GetTick();

/* Start playback */
if (ma_device_start(&device) != MA_SUCCESS) 
    {
    printf("Buzzer: Failed to start playback device.\n");
    emulator_exit(-1);
    }

/* Poll for stop */
while( buzzer_on )
    {
    if( start_time + duration <= HAL_GetTick() )
        {
        buzzer_on = false;
        }
        sleep(1);
    }

/* Stop playback */
ma_device_stop(&device);

} /* emulator_buzzer_beep_request */


/*******************************************************************************
*                                                                              *
* PROCEDURE:                                                                   * 
* 		buzzer_wave_callback                                                   *
*                                                                              *
* DESCRIPTION:                                                                 * 
*       Refill the wave buffer.                                                *
*                                                                              *
*******************************************************************************/
static void buzzer_wave_callback
    (
    ma_device* pDevice, 
    void* pOutput, 
    const void* pInput, 
    ma_uint32 frameCount
    )
{
ma_waveform* pSawWave;

EMULATOR_QUICK_ASSERT(pDevice->playback.channels == DEVICE_CHANNELS, "Invariant: Audio device channels mismatched." );

pSawWave = (ma_waveform*)pDevice->pUserData;
EMULATOR_QUICK_ASSERT(pSawWave != NULL, "Invariant: Wave buffer is a null pointer.");

ma_waveform_read_pcm_frames(pSawWave, pOutput, frameCount, NULL);

(void)pInput;   /* Unused. */

} /* buzzer_wave_callback */