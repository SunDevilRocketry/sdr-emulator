/*******************************************************************************
*
* FILE: 
* 		emulator_spi.c
*
* DESCRIPTION: 
* 		Mocks the functionality of SPI peripherals on the FC.
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
#include <stdio.h>
#include <stdlib.h>

#include "emulator.h"
#include "stm32h7xx_hal.h"
#include "flash.h"
#include "sdr_pin_defines_A0002.h"

#define FLASH_FILENAME "../../../../emulator/resources/emulator_flash.bin"
#define FLASH_TMPFILENAME FLASH_FILENAME ".tmp"

/*------------------------------------------------------------------------------
 Statics                                                         
------------------------------------------------------------------------------*/
static uint8_t flash_memory[FLASH_MAX_ADDR + 1];

static uint8_t last_flash_spi_opcode = 0x00;

/*------------------------------------------------------------------------------
 Procedure prototypes                                                      
------------------------------------------------------------------------------*/
static HAL_StatusTypeDef flash_spi_transmit(SPI_HandleTypeDef *hspi, const uint8_t *pData, uint16_t Size, uint32_t Timeout);
static HAL_StatusTypeDef flash_spi_receive(SPI_HandleTypeDef *hspi, uint8_t *pData, uint16_t Size, uint32_t Timeout);
static void atomic_flash_file_write
    (
    void
    );
static void flash_spi_delay
    (
    uint32_t num_bytes
    );

/*------------------------------------------------------------------------------
 HAL interfaces                                                       
------------------------------------------------------------------------------*/

HAL_StatusTypeDef HAL_SPI_Transmit(SPI_HandleTypeDef *hspi, const uint8_t *pData, uint16_t Size, uint32_t Timeout) {
    HAL_StatusTypeDef status = HAL_OK;
    
    if( hspi == &( FLASH_SPI ) )
        {
        status = flash_spi_transmit(hspi, pData, Size, Timeout);
        }
    
    flash_spi_delay( Size );

    return status;
}

HAL_StatusTypeDef HAL_SPI_Receive(SPI_HandleTypeDef *hspi, uint8_t *pData, uint16_t Size, uint32_t Timeout) {
    HAL_StatusTypeDef status = HAL_OK;

    if( hspi == &( FLASH_SPI ) )
        {
        status = flash_spi_receive(hspi, pData, Size, Timeout);
        }
    
    flash_spi_delay( Size );

    return status;
}

/*------------------------------------------------------------------------------
 Procedures                                                     
------------------------------------------------------------------------------*/

void emulator_flash_init
    (
    void
    )
{
/* Open or create flash file */
static FILE* flash_fp = NULL;
flash_fp = fopen( FLASH_FILENAME, "r+b");
if ( flash_fp == NULL )
    {
    printf( "Emulator Init: Flash file does not exist. Creating new.\n" );
    memset( flash_memory, 0xFF, sizeof( flash_memory ) );
    atomic_flash_file_write();
    }
else
    {
    printf( "Emulator Init: Found existing flash file.\n" );

    /* Invariant: Check size to ensure correct format. */
    fseek(flash_fp, 0L, SEEK_END);
    long size = ftell( flash_fp );
    fseek(flash_fp, 0L, SEEK_SET);

    if ( size != (FLASH_MAX_ADDR + 1) )
        {
        fprintf( stderr, "Emulator Init: Flash format appears incorrect. Please delete %s.\n", FLASH_FILENAME );
        exit(1);
        }
    else
        {
        printf( "Emulator Init: Flash format appears correct. Reading contents.\n" );
        }
    size_t read_size = fread( flash_memory, sizeof( uint8_t ), sizeof( flash_memory ), flash_fp );
    if ( read_size == sizeof( flash_memory ) )
        {
        printf( "Emulator Init: Flash memory loaded.\n" );
        }
    else
        {
        fprintf( stderr, "Emulator Init: An error occurred loading flash memory.\n" );
        fprintf( stderr, "    [DEBUG]: read_size == %ld.\n", read_size );
        exit(1);
        }

    }
fclose(flash_fp);

} /* emulator_flash_init */


uint32_t emulator_flash_write
    (
    uint8_t* data,
    uint32_t address,
    uint16_t size
    )
{
printf("    [DEBUG]: Attempting write.\n");

/* Check invariants */
if( address + size > FLASH_MAX_ADDR )
    {
    return FLASH_ADDR_OUT_OF_BOUNDS;
    }

/* Proceed with write */
memcpy( flash_memory, data, size );
atomic_flash_file_write();
// flash_spi_delay( size );

return FLASH_OK;

} /* emulator_flash_write */


uint32_t emulator_flash_read
    (
    uint8_t* data,
    uint32_t address,
    uint16_t size
    )
{
// printf("    [DEBUG]: Attempting read.\n");

/* Check invariants */
if( address + size > FLASH_MAX_ADDR )
    {
    return FLASH_ADDR_OUT_OF_BOUNDS;
    }

/* Proceed with read */
memcpy( data, flash_memory, size );
// flash_spi_delay( size ); ETS TEMP: This is slowing us pretty bad.

return FLASH_OK;

} /* emulator_flash_read */


uint32_t emulator_flash_erase
    (
    void
    )
{
printf("    [DEBUG]: Attempting erase.\n");
memset( flash_memory, 255, sizeof( flash_memory ) );
atomic_flash_file_write();

return FLASH_OK;

} /* emulator_flash_erase */


uint32_t emulator_flash_block_erase
    (
    uint32_t        flash_block_num, /* Block of flash to erase */
	uint32_t        size             /* Size of block           */
    )
{
printf("    [DEBUG]: Attempting block erase.\n");
/* Local Variables */
uint32_t true_size = 0;
uint32_t start_erase = 0;
uint32_t end_erase = 0;

switch( size )
    {
    case FLASH_BLOCK_4K:
        true_size = 0x1000;  
        break;  
    case FLASH_BLOCK_32K:
        true_size = 0x8000;
        break;
    case FLASH_BLOCK_64K:
        true_size = 0x10000;
        break;
    default:
        return FLASH_FAIL;
        break;
    }

start_erase = flash_block_num * true_size;
end_erase = ( ( flash_block_num + 1 ) * true_size) - 1;
printf( "    [DEBUG]: Block Erase -- Start erase at %X, end erase at %X.\n", start_erase, end_erase );

/* Check Invariants */
if ( ( start_erase >= end_erase )
  || ( end_erase >= FLASH_MAX_ADDR ) )
    {
    return FLASH_ADDR_OUT_OF_BOUNDS;
    }

/* POINTER ARITHMETIC; USE CAUTION */
memset( flash_memory + start_erase, 255, true_size );
atomic_flash_file_write();

return FLASH_OK;

} /* emulator_flash_block_erase */


static HAL_StatusTypeDef flash_spi_transmit(SPI_HandleTypeDef *hspi, const uint8_t *pData, uint16_t Size, uint32_t Timeout) 
{

if ( Size == 1 )
    {
    last_flash_spi_opcode = *pData;
    }
else
    {
    last_flash_spi_opcode = 0x00;
    }

return HAL_OK;
}

static HAL_StatusTypeDef flash_spi_receive(SPI_HandleTypeDef *hspi, uint8_t *pData, uint16_t Size, uint32_t Timeout) 
{
/* handle flash opcode*/
if ( last_flash_spi_opcode == FLASH_OP_HW_RDSR )
    {
    *pData = 0x00; /* Anything but 0xFF */
    return HAL_OK;
    }

if ( Size == 1 )
    {
    last_flash_spi_opcode = *pData;
    }
else
    {
    last_flash_spi_opcode = 0x00;
    }

return HAL_OK;
}

static void atomic_flash_file_write
    (
    void
    )
{
printf("    [DEBUG]: Atomic Write\n");
printf("    [DEBUG]: Checking Hang.\n");
FILE* tmp_fp = fopen( FLASH_TMPFILENAME, "w+" );
fwrite( flash_memory, sizeof( uint8_t ), sizeof( flash_memory ), tmp_fp );
fclose( tmp_fp );
if ( rename( FLASH_TMPFILENAME, FLASH_FILENAME ) == 0 )
    {
    printf("    [DEBUG]: Rename Success.\n");
    return;
    }
else
    {
    /* WARNING: NOT ATOMIC, DATA LOSS POSSIBLE */
    printf("    [DEBUG]: Non-atomic.\n");
    remove( FLASH_FILENAME );
    rename( FLASH_TMPFILENAME, FLASH_FILENAME );
    }
}


static void flash_spi_delay
    (
    uint32_t num_bytes
    )
{
float delay_time = 0.26 + (num_bytes * 0.02);
HAL_Delay( (uint32_t)(delay_time + 0.5) );

} /* flash_spi_delay */