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
#include <errno.h>

/* CYGWIN stuff */
#include <sys/fcntl.h>
#include <sys/types.h>
#include <sys/unistd.h>
#include <sys/mman.h>

#include "emulator.h"
#include "stm32h7xx_hal.h"
#include "flash.h"
#include "sdr_pin_defines_A0002.h"

#define FLASH_FILENAME "../../../../emulator/resources/emulator_flash.bin"
#define FLASH_TMPFILENAME FLASH_FILENAME ".tmp"

/* Note that this is implicitly in bytes because flash_memory is a byte array*/
#define FLASH_FILESIZE FLASH_MAX_ADDR + 1

/*------------------------------------------------------------------------------
 Statics                                                         
------------------------------------------------------------------------------*/
static uint8_t *flash_memory;

static uint8_t last_flash_spi_opcode = 0x00;

/*------------------------------------------------------------------------------
 Procedure prototypes                                                      
------------------------------------------------------------------------------*/
static HAL_StatusTypeDef flash_spi_transmit(SPI_HandleTypeDef *hspi, const uint8_t *pData, uint16_t Size, uint32_t Timeout);
static HAL_StatusTypeDef flash_spi_receive(SPI_HandleTypeDef *hspi, uint8_t *pData, uint16_t Size, uint32_t Timeout);
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
/* Creates a new flash file. Overwrites if already exists (O_TRUNC) */
/* Do note that the created file has full permissions */
int flashFileFd = open(FLASH_FILENAME, O_CREAT | O_RDWR | O_TRUNC, S_IRWXO | S_IRWXG | S_IRWXU);

if ( flashFileFd == -1 ) 
    {
    printf("Emulator Init: Flash file failed to open with errno %d", errno);
    exit(1);
    }

/* Write ones and make file size of flash. */
/* Kind of an ugly method but I'm not sure if there's a way to set */
/* files to a specific size. Shouldn't be too bad since this is on the stack */
uint8_t tmpInitBytes[FLASH_FILESIZE]; 

memset(tmpInitBytes, 0xFF, FLASH_FILESIZE);

off_t fileSeekSize = lseek(flashFileFd, 0, SEEK_END);

ssize_t writeFileSize  = -1;

if ( fileSeekSize < FLASH_FILESIZE ) 
    {
    writeFileSize = write(flashFileFd, tmpInitBytes, FLASH_FILESIZE);
    }
else 
    {
    writeFileSize = FLASH_FILESIZE;
    }

if ( writeFileSize != FLASH_FILESIZE )
    {
    printf("Emulator Init: Failed to write to file with errno %d\n", errno);
    exit(1);
    }

/* Reset file offset to zero */
lseek(flashFileFd, 0, SEEK_SET);

/* Note: consider looking into msync to control when file is updated */
flash_memory = mmap(NULL, FLASH_FILESIZE, PROT_READ | PROT_WRITE, MAP_SHARED, flashFileFd, 0);

if ( flash_memory == MAP_FAILED ) 
    {
    printf("Emulator Init: Flash file mmap failed with errno %d\n", errno);
    exit(1);
    }

/* File can be closed without invalidating the mapping, per the man */
int closeRet = close(flashFileFd);
if ( closeRet == -1 ) 
    {
    printf("Emulator Init: Failed to close inital flash file with errno %d\n", errno);
    exit(1);
    }

printf("Emulator Init: Successfully mapped %s to memory\n", FLASH_FILENAME);

} /* emulator_flash_init */

void emulator_flash_cleanup
    (
    void
    )
{

munmap(flash_memory, FLASH_FILESIZE);

} /* emulator_flash_cleanup */


uint32_t emulator_flash_write
    (
    uint8_t* data,
    uint32_t address,
    uint16_t size
    )
{
printf("    [DEBUG]: Attempting write.\n");

/* Check invariants */
if( address + ( size - 1 ) > FLASH_MAX_ADDR )
    {
    printf("Flash Write: OOB write at %x with size %d\n", address, size);
    return FLASH_ADDR_OUT_OF_BOUNDS;
    }

/* Proceed with write */
/* Offset base buffer address by desired write address */
memcpy( flash_memory + address, data, size );
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
printf("[FLASH DEBUG]: Read at %x with size %d\n", address, size);

/* Check invariants */
if( address + ( size - 1 ) > FLASH_MAX_ADDR )
    {
    printf("Flash Read: OOB read at %x with size %d\n", address, size);
    printf("Flash Read: Max addr would have been %x.\n", address + size);
    return FLASH_ADDR_OUT_OF_BOUNDS;
    }

/* Proceed with read */
memcpy( data, &(flash_memory[address]), size );
// flash_spi_delay( size ); ETS TEMP: This is slowing us pretty bad.

return FLASH_OK;

} /* emulator_flash_read */


uint32_t emulator_flash_erase
    (
    void
    )
{
printf("    [DEBUG]: Attempting erase.\n");
memset( flash_memory, 255, FLASH_FILESIZE );

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

static void flash_spi_delay
    (
    uint32_t num_bytes
    )
{
float delay_time = 0.26 + (num_bytes * 0.02);
HAL_Delay( (uint32_t)(delay_time + 0.5) );

} /* flash_spi_delay */