/*******************************************************************************
*
* FILE: 
* 		mat4.c
*
* DESCRIPTION: 
*       Implementations of 4x4 matrix operations
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
 Standard Includes                                                                    
------------------------------------------------------------------------------*/
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/*------------------------------------------------------------------------------
 Project Includes
------------------------------------------------------------------------------*/
#include "math/lin.h"

/*------------------------------------------------------------------------------
 Functions 
------------------------------------------------------------------------------*/

/*******************************************************************************
*                                                                              *
* PROCEDURE:                                                                   * 
* 		mat4Identity                                                           *
*                                                                              *
* DESCRIPTION:                                                                 * 
*       Returns a mat4 type with intialized to the identity matrix             *
*                                                                              *
*******************************************************************************/
mat4 mat4Identity
    (
    void
    )
{
mat4 retMat4 = {
    .data = {1, 0, 0, 0,
             0, 1, 0, 0,
             0, 0, 1, 0,
             0, 0, 0, 1}
};

return retMat4;

} /* mat4Identity */


/*******************************************************************************
*                                                                              *
* PROCEDURE:                                                                   * 
* 		mat4Mult                                                               *
*                                                                              *
* DESCRIPTION:                                                                 * 
*       Returns a mat4 equal to the product of a * b                           *
*                                                                              *
*******************************************************************************/
GRAPHICS_OPTIMIZED_PROCEDURE /* defines optimization attributes */
mat4 mat4Mult
    (
    const mat4 a, 
    const mat4 b
    )
{
mat4 retMat = {};

/* I LOVE nested for loops and pointer arithmetic!!!! */
for (size_t row = 0; row < 4; row++)
{
for (size_t col = 0; col < 4; col++)
    {
    *(retMat.data + col * 4 + row) =
        *(a.data + 0 * 4 + row) * *(b.data + col * 4 + 0) +
        *(a.data + 1 * 4 + row) * *(b.data + col * 4 + 1) +
        *(a.data + 2 * 4 + row) * *(b.data + col * 4 + 2) +
        *(a.data + 3 * 4 + row) * *(b.data + col * 4 + 3);
    }
}

return retMat;

} /* mat4Mult */

/*******************************************************************************
*                                                                              *
* PROCEDURE:                                                                   * 
* 		mat4MultScalar                                                         *
*                                                                              *
* DESCRIPTION:                                                                 * 
*       Returns a mat4 multiplied component-wise by the given scalar           *
*                                                                              *
*******************************************************************************/
GRAPHICS_OPTIMIZED_PROCEDURE /* defines optimization attributes */
mat4 mat4MultScalar
    (
    const mat4 a, 
    float scalar
    )
{
mat4 retMat = a;

for (size_t col = 0; col < 4; col++)
{
for (size_t row = 0; row < 4; row++)
    {
        *(retMat.data + col * 4 + row) *= scalar;
    }
}

return retMat;

} /* mat4MultScalar */

/*******************************************************************************
*                                                                              *
* PROCEDURE:                                                                   * 
* 		mat4Add                                                                *
*                                                                              *
* DESCRIPTION:                                                                 * 
*       Returns a mat4 equal to the component-wise sum of of a + b             *
*                                                                              *
*******************************************************************************/
GRAPHICS_OPTIMIZED_PROCEDURE /* defines optimization attributes */
mat4 mat4Add
    (
    const mat4 a, 
    const mat4 b
    )
{
mat4 retMat = {};

for (size_t col = 0; col < 4; col++) 
{
for (size_t row = 0; row < 4; row++) 
    {
       *(retMat.data + col * 4 + row) = *(a.data + col * 4 + row) + *(b.data + col * 4 + row);
    }
}

return retMat;

} /* mat4Add */

/*******************************************************************************
*                                                                              *
* PROCEDURE:                                                                   * 
* 		mat4RotY                                                               *
*                                                                              *
* DESCRIPTION:                                                                 * 
*       Returns a mat4 representing a rotaiton of the passed angle about the   *
*       y-axis                                                                 *
*                                                                              *
*******************************************************************************/
GRAPHICS_OPTIMIZED_PROCEDURE /* defines optimization attributes */
mat4 mat4RotY
    (
    float angle
    )
{
mat4 rotMat = mat4Identity();
rotMat.data[0] = cos(angle);
rotMat.data[2] = -sin(angle);
rotMat.data[8] = sin(angle);
rotMat.data[10] = cos(angle);

return rotMat;

} /* mat4RotY */

/*******************************************************************************
*                                                                              *
* PROCEDURE:                                                                   * 
* 		mat4AxisAngle                                                          *
*                                                                              *
* DESCRIPTION:                                                                 * 
*       Uses the Rodrigues rotation formula to return a mat4 representing a    *
*       rotation of angle radians about the passed axis                        *
*                                                                              *
*******************************************************************************/
GRAPHICS_OPTIMIZED_PROCEDURE /* defines optimization attributes */
mat4 mat4AxisAngle
    (
    vec3 axis, 
    float angle
    )
{
mat4 I = mat4Identity();
mat4 A;
memset(A.data, 0, sizeof(float) * 16);

A.data[1] = axis.members.z;
A.data[2] = -axis.members.y;

A.data[4] = -axis.members.z;
A.data[6] = axis.members.x;

A.data[8] = axis.members.y;
A.data[9] = -axis.members.x;

mat4 A2 = mat4MultScalar(mat4Mult(A, A), 1 - cos(angle));
A = mat4MultScalar(A, sin(angle));

return mat4Add(I, mat4Add(A2, A));

} /* mat4AxisAngle */

/*******************************************************************************
*                                                                              *
* PROCEDURE:                                                                   * 
* 		mat4Translation                                                        *
*                                                                              *
* DESCRIPTION:                                                                 * 
*       Returns a mat4 representing a translation of (x, y, z)                 *
*                                                                              *
*******************************************************************************/
GRAPHICS_OPTIMIZED_PROCEDURE /* defines optimization attributes */
mat4 mat4Translation
    (
    float x, 
    float y, 
    float z
    )
{
mat4 translation = mat4Identity();

translation.data[12] = x;
translation.data[13] = y;
translation.data[14] = z;

return translation;

} /* mat4Translation */

/*******************************************************************************
*                                                                              *
* PROCEDURE:                                                                   * 
* 		mat4LookAt                                                             *
*                                                                              *
* DESCRIPTION:                                                                 * 
*       Uses the Gram-Schmidt process to create a matrix representing the      *
*       camera. Do note that it is erroneous to have a up vector parallel to   *
*       the camera's look vector                                               *
*                                                                              *
*******************************************************************************/
GRAPHICS_OPTIMIZED_PROCEDURE /* defines optimization attributes */
mat4 mat4LookAt
    (
    vec3 position, 
    vec3 target,
    vec3 up
    ) 
{
/* Camera looks down the -Z axis, so this finds the negative look vector */
vec3 zAxis = vec3Normalize(vec3Sub(position, target));
vec3 xAxis = vec3Normalize(vec3Cross(up, zAxis));
vec3 cameraUp = vec3Normalize(vec3Cross(zAxis, xAxis));
/* Do note that this process has a side effect of causing issues when the up vector is the same as the zAxis (-lookDirection) */

mat4 lookAt = mat4Identity(); 
/* First column, X_1, Y_1, Z_1 */
lookAt.data[0] = xAxis.members.x;
lookAt.data[1] = cameraUp.members.x;
lookAt.data[2] = zAxis.members.x;

/* Second column, X_2, Y_2, Z_2 */
lookAt.data[4] = xAxis.members.y;
lookAt.data[5] = cameraUp.members.y;
lookAt.data[6] = zAxis.members.y;

/* Third column, X_3, Y_3, Z_3 */
lookAt.data[8] = xAxis.members.z;
lookAt.data[9] = cameraUp.members.z;
lookAt.data[10] = zAxis.members.z;

/* Fourth column, -Px, -Py, -Pz (instead of moving the 'camera' by some vector, we just move everything else by the negation of that vector) */
lookAt.data[12] = -position.members.x;
lookAt.data[13] = -position.members.y;
lookAt.data[14] = -position.members.z;

return lookAt;

} /* mat4LookAt */

/*******************************************************************************
*                                                                              *
* PROCEDURE:                                                                   * 
* 		mat4Proj                                                               *
*                                                                              *
* DESCRIPTION:                                                                 * 
*       Returns a perspective projection matrix initialized with the           *
*       passed parameters.                                                     *
*                                                                              *
*******************************************************************************/
GRAPHICS_OPTIMIZED_PROCEDURE /* defines optimization attributes */
mat4 mat4Proj
    (
    float fov,
    float aspect,
    float near,
    float far
    ) 
{
/* Dont really understand the z component modifications from a mathematical standpoint. Its some kind of non-linear mapping to provide more accuracy to vertices closer to the camera */

float xMult = 1/(aspect*tan(fov/2.0));
float yMult = 1/tan(fov/2.0);
float zMult = -((far + near)/(far - near));

mat4 proj = {};
proj.data[0] = xMult;
proj.data[5] = yMult;
proj.data[10] = zMult;
proj.data[11] = -1;
proj.data[14] = -(2*far*near)/(far-near);

return proj;

} /* mat4Proj */


/*******************************************************************************
* END OF FILE                                                                  *
*******************************************************************************/
