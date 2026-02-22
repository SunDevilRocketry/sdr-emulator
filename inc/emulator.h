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
 Macros  
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

/* emulator.h */
void guisock_put
    (
    const char* message,
    size_t size
    );

/* emulator_timer.c */
void emulator_start_timers
    (
    void
    );

void emulator_buzzer_beep_request(uint32_t duration);

/* emulator_error.c */
void emulator_setup_error
    (
    void
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

/* emulator_gui.c entry point */
void emulator_gui_main
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

#ifdef __cplusplus
}
#endif

#endif /* __EMULATOR_H */


/*******************************************************************************
* END OF FILE                                                                  *
*******************************************************************************/