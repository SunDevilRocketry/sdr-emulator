/*******************************************************************************
*
* FILE: 
* 		vec3.c
*
* DESCRIPTION: 
*       Implementations of 3 component vector operations
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
* 		vec3New                                                                *
*                                                                              *
* DESCRIPTION:                                                                 * 
*       Returns a vec3 type with corresponding compenents intialized to the    *
*       passed values                                                          *
*                                                                              *
*******************************************************************************/
GRAPHICS_OPTIMIZED_PROCEDURE /* defines optimization attributes */
vec3 vec3New
    (
    const float x, 
    const float y, 
    const float z
    ) 
{

vec3 new; 
new.data[0] = x;
new.data[1] = y;
new.data[2] = z;

return new;
} /* vec3New */

/*******************************************************************************
*                                                                              *
* PROCEDURE:                                                                   * 
* 		vec3Dot                                                                *
*                                                                              *
* DESCRIPTION:                                                                 * 
*       Returns the dot (scalar) product of the two passed vec3s               *
*                                                                              *
*******************************************************************************/
GRAPHICS_OPTIMIZED_PROCEDURE /* defines optimization attributes */
float vec3Dot
    (
    const vec3 a, 
    const vec3 b
    )
{
return 
    (a.data[0])*(b.data[0]) + 
    (a.data[1])*(b.data[1]) + 
    (a.data[2])*(b.data[2]);

} /* vec3Dot */

/*******************************************************************************
*                                                                              *
* PROCEDURE:                                                                   * 
* 		vec3Cross                                                              *
*                                                                              *
* DESCRIPTION:                                                                 * 
*       Returns the cross product of the passed vec3s as such: a X b           *
*                                                                              *
*******************************************************************************/
GRAPHICS_OPTIMIZED_PROCEDURE /* defines optimization attributes */
vec3 vec3Cross
    (
    const vec3 
    a, 
    const vec3 b
    )
{
vec3 retVec;

retVec.data[0] = (a.data[1])*(b.data[2]) - (a.data[2])*(b.data[1]);
retVec.data[1] = (a.data[2])*(b.data[0]) - (a.data[0])*(b.data[2]);
retVec.data[2] = (a.data[0])*(b.data[1]) - (a.data[1])*(b.data[0]);

return retVec;

} /* vec3Cross */

/*******************************************************************************
*                                                                              *
* PROCEDURE:                                                                   * 
* 		vec3Magnitude                                                          *
*                                                                              *
* DESCRIPTION:                                                                 * 
*       Returns the magnitude of the passed vec3                               *
*                                                                              *
*******************************************************************************/
GRAPHICS_OPTIMIZED_PROCEDURE /* defines optimization attributes */
float vec3Magnitude
    (
    const vec3 vec
    )
{
    return sqrtf(powf(vec.data[0], 2) + powf(vec.data[1], 2) + powf(vec.data[2], 2));

} /* vec3Magnitude */

/*******************************************************************************
*                                                                              *
* PROCEDURE:                                                                   * 
* 		vec3Normalize                                                          *
*                                                                              *
* DESCRIPTION:                                                                 * 
*       Returns a vec3 of identical direction to the passed vec, but of        *
*       magnitude 1.                                                           *
*                                                                              *
*******************************************************************************/
GRAPHICS_OPTIMIZED_PROCEDURE /* defines optimization attributes */
vec3 vec3Normalize
    (
    const vec3 vec
    ) 
{
vec3 retVec;

float mag = vec3Magnitude(vec);
retVec.data[0] = vec.data[0] / mag;
retVec.data[1] = vec.data[1] / mag;
retVec.data[2] = vec.data[2] / mag;

return retVec;

} /* vec3Normalize */ 

/*******************************************************************************
*                                                                              *
* PROCEDURE:                                                                   * 
* 		vec3Sub                                                                *
*                                                                              *
* DESCRIPTION:                                                                 * 
*       Returns the difference resulting from the component-wise subtraction   *
*       of a - b                                                               *
*                                                                              *
*******************************************************************************/
GRAPHICS_OPTIMIZED_PROCEDURE /* defines optimization attributes */
vec3 vec3Sub
    (
    const vec3 a, 
    const vec3 b
    )
{
vec3 difference;

difference.data[0] = a.members.x - b.members.x;
difference.data[1] = a.members.y - b.members.y;
difference.data[2] = a.members.z - b.members.z;

return difference;

} /* vec3Sub */

/*******************************************************************************
*                                                                              *
* PROCEDURE:                                                                   * 
* 		vec3MultScalar                                                         *
*                                                                              *
* DESCRIPTION:                                                                 * 
*       Returns a vector resulting from multiplying the passed vector by the   *
*       passed scalar                                                          *
*                                                                              *
*******************************************************************************/
GRAPHICS_OPTIMIZED_PROCEDURE /* defines optimization attributes */
vec3 vec3MultScalar
    (
    const vec3 a, 
    float scalar) 
{
vec3 retVec;
retVec.data[0] = a.data[0] * scalar;
retVec.data[1] = a.data[1] * scalar;
retVec.data[2] = a.data[2] * scalar;
return retVec;

} /* vec3MultScalar */

/*******************************************************************************
* END OF FILE                                                                  *
*******************************************************************************/
