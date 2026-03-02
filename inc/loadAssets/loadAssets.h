/*******************************************************************************
*
* FILE: 
* 		loadAssets.h
*
* DESCRIPTION: 
*       Provides standard structure for handling vertex data after parsing from file.
*       Provides function prototypes for parsing vertex data from file.
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

#ifndef LOAD_ASSETS_H_
#define LOAD_ASSETS_H_

#ifdef __cplusplus
extern "C" {
#endif


/*------------------------------------------------------------------------------
 Standard Includes                                                                    
------------------------------------------------------------------------------*/

#include <stddef.h>

/*------------------------------------------------------------------------------
 Structs
------------------------------------------------------------------------------*/


struct fileVertexData {
    /* TODO: Take *DataSize out of this struct */
    size_t vertexDataSize; /* Used in parsing to contain full size of array; same as data count otherwise */
    size_t vertexDataCount;
    float *vertexData;

    size_t faceIndexDataSize; /* Used in parsing to contain full size of array; same as data count otherwise */
    size_t faceIndexDataCount;
    unsigned int *faceIndexData;
};

/*------------------------------------------------------------------------------
 Function prototypes                                             
------------------------------------------------------------------------------*/

struct fileVertexData loadVertexDataFromOBJ
    (
    const char* filepath
    );

#ifdef __cplusplus
}
#endif

#endif /* LOAD_ASSETS_H_ */

/*******************************************************************************
* END OF FILE                                                                  *
*******************************************************************************/
