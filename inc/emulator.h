/*******************************************************************************
*
* FILE: 
* 		emulator.h
*
* DESCRIPTION: 
* 		Main header file for SDR hardware emulator
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __EMULATOR_H
#define __EMULATOR_H

#ifdef __cplusplus
extern "C" {
#endif


/*------------------------------------------------------------------------------
 Standard Includes                                                                    
------------------------------------------------------------------------------*/
#include <stdint.h>
#include <stdbool.h>

/*------------------------------------------------------------------------------
 Project Includes  
------------------------------------------------------------------------------*/


/*------------------------------------------------------------------------------
 Typedefs
------------------------------------------------------------------------------*/


/*------------------------------------------------------------------------------
 Global Variables                                             
------------------------------------------------------------------------------*/


/*------------------------------------------------------------------------------
 Exported function prototypes                                             
------------------------------------------------------------------------------*/


/*------------------------------------------------------------------------------
 Function prototypes                                             
------------------------------------------------------------------------------*/

/* firmware entry point */
int main_fut(void);

/* emulator.c */
void emulator_exit
    (
    int exitCode
    );

/* emulator_audio.c */
void emulator_buzzer_init
    (
    void
    );

void emulator_buzzer_teardown
    (
    void
    );

void emulator_buzzer_beep_request
    (
    uint32_t duration
    );

/* emulator_timer.c */
void emulator_start_timers
    (
    void
    );

/* emulator_error.c */
void emulator_setup_error
    (
    void
    );

void emulator_internal_error
    (
    const char* file,
    const int line,
    const char* msg
    );

/* emulator_i2c.c */
void* emulator_i2c_it_listener
    (
    void* arg
    );

/* emulator_spi.c */
void emulator_flash_init
    (
    void
    );

/* emulator_gui.c */
void emulator_gui_main
    (
    void
    );

void set_gui_status_led
    (
    const float r, 
    const float g, 
    const float b
    );

void set_ignite_flag
    (
    bool status
    );

void emulator_gui_init
    (
    void
    );

void emulator_gui_teardown
    (
    void
    );

uint32_t emulator_flash_write
    (
    uint8_t* data,
    uint32_t address,
    uint16_t size
    );


uint32_t emulator_flash_read
    (
    uint8_t* data,
    uint32_t address,
    uint16_t size
    );

uint32_t emulator_flash_erase
    (
    void
    );

uint32_t emulator_flash_block_erase
    (
    uint32_t      flash_block_num, /* Block of flash to erase */
	uint32_t      size             /* Size of block           */
    );

/* emulator_uart.c */
bool emulator_prompt_and_open_serial_port
    (
    void
    );

void* emulator_gps_it_listener
    (
    void* arg
    );

/*------------------------------------------------------------------------------
 Shortcut macros  
------------------------------------------------------------------------------*/
#define EMULATOR_ERROR( msg ) emulator_internal_error( __FILE__, __LINE__, msg )
#define EMULATOR_QUICK_ASSERT( a, msg ) do { if (!(a)) EMULATOR_ERROR( msg ); } while(0) /* gross but it works*/

#ifdef __cplusplus
}
#endif

#endif /* __EMULATOR_H */


/*******************************************************************************
* END OF FILE                                                                  *
*******************************************************************************/