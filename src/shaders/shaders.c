/**
 * @file readShader.c
 *
 * @brief Implemtations of shader handling functions
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
 */

/*------------------------------------------------------------------------------
 Standard Includes                                                         
------------------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <sys/fcntl.h>
#include <sys/stat.h>
#include <stddef.h>
#include <errno.h>
#include <unistd.h>

/*------------------------------------------------------------------------------
 Project Includes                                                         
------------------------------------------------------------------------------*/
#include "shaders/shaders.h"

/*------------------------------------------------------------------------------
 Functions
------------------------------------------------------------------------------*/

/*******************************************************************************
*                                                                              *
* PROCEDURE:                                                                   * 
* 		readShaderSource                                                       *
*                                                                              *
* DESCRIPTION:                                                                 * 
*       Reads the file at the given path and returns a NULL-terminated string  *
*       containing that file's contents. File must exist and caller is         * 
*       responsible for freeing the returned string.                           *
*                                                                              *
*******************************************************************************/
char* readShaderSource
    (
    const char* path
    ) 
{

int shaderfd = open(path, O_RDONLY);

if (shaderfd == -1) {
    fprintf(stderr, "Failed to read shader source %s with errno %d\n", path, errno);
    return NULL;
}

struct stat stat;
int fstatStatus = fstat(shaderfd, &stat);

if (fstatStatus == -1) {
    fprintf(stderr, "Failed to get stat of shader file %s with errno %d\n", path, errno);
    return NULL;
}


char* data = malloc(sizeof(char) * stat.st_size + 1);
if (data == NULL) {
    fprintf(stderr, "Failed to allocate buffer for shader source %s\n", path);
}

ssize_t bytesRead = read(shaderfd, data, stat.st_size);
data[stat.st_size] = '\0'; // null term

if (bytesRead != stat.st_size || bytesRead == -1) {
    fprintf(stderr, "Bad read of shader file %s with errno %d\n", path, errno);
    free(data);
    return NULL;
}

return data;

} /* readShaderSource */

/*******************************************************************************
*                                                                              *
* PROCEDURE:                                                                   * 
* 		genShaderFromSource                                                    *
*                                                                              *
* DESCRIPTION:                                                                 * 
*       Creates and returns an OpenGL shader object compiled from the passed   *
*       file path and of the passed shader type.                               *
*                                                                              *
*******************************************************************************/
GLuint genShaderFromSource
    (
    const char* path,
    GLenum shaderType
    )
{
int success;
char infoLog[512];

const char* shaderCode = readShaderSource(path);
if (shaderCode == NULL) 
    {
    return 0;
    }
    
GLuint shaderID = glCreateShader(shaderType);
glShaderSource(shaderID, 1, &shaderCode, NULL); 
glCompileShader(shaderID);
free((void*)shaderCode); /* The glShaderSource function requires type const char*, but GCC gets mad when you try to free a const pointer. Cast to fix this issue */

glGetShaderiv(shaderID, GL_COMPILE_STATUS, &success);
if (!success) 
    {
    glGetShaderInfoLog(shaderID, 512, NULL, infoLog);
    fprintf(stderr, "Shader %s compilation failed:\n \t%s\n", path, infoLog);
    }

return shaderID;
} /* genShaderFromSource */

/*******************************************************************************
*                                                                              *
* PROCEDURE:                                                                   * 
* 		genShaderFromSource                                                    *
*                                                                              *
* DESCRIPTION:                                                                 * 
*       Creates and returns an OpenGL shader program with vertex and fragment  *
*       stages corresponding to the passed shader paths. Return value of       *
*       zero indicates error as per the OpenGL specification                   *
*                                                                              *
*******************************************************************************/
GLuint genShaderProgramFromSources
    (
    const char* vertexShaderPath,
    const char* fragmentShaderPath
    ) 
{

GLuint vShader = genShaderFromSource(vertexShaderPath, GL_VERTEX_SHADER);
if (!vShader)
    {
    return 0;
    }

GLuint fShader = genShaderFromSource(fragmentShaderPath, GL_FRAGMENT_SHADER);
if (!fShader) 
    {
    glDeleteShader(vShader);
    return 0;
    }

GLuint program = glCreateProgram();
if (!program) 
    {
    glDeleteShader(vShader);
    glDeleteShader(fShader);
    return 0;
    }

glAttachShader(program, vShader);
glAttachShader(program, fShader);
glLinkProgram(program);

glDeleteShader(vShader);
glDeleteShader(fShader);

int success;
glGetProgramiv(program, GL_LINK_STATUS, &success);
if (!success) 
    {
    char infoLog[512];
    glGetProgramInfoLog(program, 512, NULL, infoLog);
    fprintf(stderr, "Shader linkage failed:\n \t%s\n", infoLog);
    }

return program;
} /* genShaderProgramFromSources */

/*******************************************************************************
* END OF FILE                                                                  *
*******************************************************************************/
