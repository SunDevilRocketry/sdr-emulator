/*******************************************************************************
*
* FILE: 
* 		readShader.c
*
* DESCRIPTION: 
* 		Implementations of shader handling functions
*       
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
#include <stdio.h>
#include <stdlib.h>
#include <sys/fcntl.h>
#include <sys/stat.h>
#include <stddef.h>
#include <errno.h>
#include <unistd.h>

#include "shaders/readShader.h"

/*------------------------------------------------------------------------------
 Procedures                                                     
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
const char* readShaderSource
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

