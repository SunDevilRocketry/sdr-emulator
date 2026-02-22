/*******************************************************************************
*
* FILE: 
* 		readShader.h
*
* DESCRIPTION: 
* 		Header containing prototypes for shader processing functions
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

#ifndef READ_SHADER_H_
#define READ_SHADER_H_

#ifdef __cplusplus
extern "C" {
#endif

/*------------------------------------------------------------------------------
 Standard Includes                                                                    
------------------------------------------------------------------------------*/


/*------------------------------------------------------------------------------
 Project Includes  
------------------------------------------------------------------------------*/
#include "glad/gl.h"

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

const char* readShaderSource
    (
    const char* path
    );

GLuint genShaderFromSource
    (
    const char* path,
    GLenum shaderType
    );

GLuint genShaderProgramFromSources
    (
    const char* vertexShaderPath,
    const char* fragmentShaderPath
    );

#ifdef __cplusplus
}
#endif


#endif /* READ_SHADER_H_ */

/*******************************************************************************
* END OF FILE                                                                  *
*******************************************************************************/
