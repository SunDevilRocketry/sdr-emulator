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

// The way I have things set up right now kind of binds our mesh objects to the
// encoding of obj files. Not really my goal. 
//
// 
// The new parser will now just return an array of mesh objects with data interleaved in 
// the expected format. 
//

struct fileMaterials {
    unsigned int numIndiciesUsingMat;
    float r;
    float g;
    float b;
};

struct fileVertexData {
    struct fileMaterials* fileMaterialsData;
    float *vertexData;
    float *vertexNormalsData;
    unsigned int *faceIndexData;
    unsigned int *vertexNormalsIndices;
};

struct meshObject {
    char objName[512];
    // [x, y, z][nx, ny, nz][r, g, b]
    float* vertexData;
};

/*------------------------------------------------------------------------------
 Function prototypes                                             
------------------------------------------------------------------------------*/

/* loadOBJ.c */
struct meshObject* loadVertexDataFromOBJ
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
