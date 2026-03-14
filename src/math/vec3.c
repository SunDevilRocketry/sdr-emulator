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
vec3 vec3New
    (
    const float x, 
    const float y, 
    const float z
    ) 
{

vec3 new; 
#ifdef __SSE__ 
new._data = _mm_setr_ps(x,y,z,0);
#else
new.data[0] = x;
new.data[1] = y;
new.data[2] = z;
#endif

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
float vec3Dot
    (
    const vec3 a, 
    const vec3 b
    )
{
#ifdef __SSE4_1__ 
__m128 result = _mm_dp_ps(a._data, b._data, 0xF1);
return result[0];
#else
return 
    (a.data[0])*(b.data[0]) + 
    (a.data[1])*(b.data[1]) + 
    (a.data[2])*(b.data[2]);
#endif
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
vec3 vec3Cross
    (
    const vec3 
    a, 
    const vec3 b
    )
{
vec3 retVec;
#ifdef __SSE__
/* Note that shuffle operates as follows: 
 * result.data[0] = a.data[shuffleIndices[3]] 
 * result.data[1] = a.data[shuffleIndices[2]]
 * result.data[2] = a.data[shuffleIndices[1]]
 * result.data[3] = a.data[shuffleIndices[0]] 
 * */
__m128 ayazax_0 = _mm_shuffle_ps(a._data, a._data, _MM_SHUFFLE(3, 0, 2, 1));
__m128 bzbxby_0 = _mm_shuffle_ps(b._data, b._data, _MM_SHUFFLE(3, 1, 0, 2));;

__m128 azaxay_0 = _mm_shuffle_ps(a._data, a._data, _MM_SHUFFLE(3, 1, 0, 2));;
__m128 bybzbx_0 = _mm_shuffle_ps(b._data, b._data, _MM_SHUFFLE(3, 0, 2, 1));;

__m128 m1Prod = _mm_mul_ps(ayazax_0, bzbxby_0);
__m128 m2Prod = _mm_mul_ps(azaxay_0, bybzbx_0);

retVec._data = _mm_sub_ps(m1Prod, m2Prod);

#else

retVec.data[0] = (a.data[1])*(b.data[2]) - (a.data[2])*(b.data[1]);
retVec.data[1] = (a.data[2])*(b.data[0]) - (a.data[0])*(b.data[2]);
retVec.data[2] = (a.data[0])*(b.data[1]) - (a.data[1])*(b.data[0]);

#endif

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
float vec3Magnitude
    (
    const vec3 vec
    )
{
#ifdef __SSE3__
    /* No pow instr, just mult by self */
    /* might actually be slower than generic implementation lol */
    __m128 tmp = _mm_mul_ps(vec._data, vec._data);
    __m128 hsum = _mm_hadd_ps(tmp, tmp);
    hsum = _mm_hadd_ps(hsum, hsum);

    return sqrtf(hsum[0]);

#else
    return sqrtf(powf(vec.data[0], 2) + powf(vec.data[1], 2) + powf(vec.data[2], 2));
#endif

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
vec3 vec3Normalize
    (
    const vec3 vec
    ) 
{
vec3 retVec;
#ifdef __SSE3__

/* Inline code from magnitude to avoid re-casting to __m128 */
__m128 mag = _mm_mul_ps(vec._data, vec._data);
mag = _mm_hadd_ps(mag, mag);
mag = _mm_sqrt_ps(_mm_hadd_ps(mag, mag));
retVec._data = _mm_div_ps(vec._data, mag);

#else

float mag = vec3Magnitude(vec);
retVec.data[0] = vec.data[0] / mag;
retVec.data[1] = vec.data[1] / mag;
retVec.data[2] = vec.data[2] / mag;

#endif

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
vec3 vec3Sub
    (
    const vec3 a, 
    const vec3 b
    )
{
vec3 difference;
#ifdef __SSE__
/* mfw when single instruction multiple data */
difference._data = _mm_sub_ps(a._data, b._data);
#else
difference.data[0] = a.members.x - b.members.x;
difference.data[1] = a.members.y - b.members.y;
difference.data[2] = a.members.z - b.members.z;
#endif

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
vec3 vec3MultScalar
    (
    const vec3 a, 
    float scalar) 
{
vec3 retVec;
#ifdef __SSE__
__m128 scale_r = _mm_set1_ps(scalar);
retVec._data = _mm_mul_ps(a._data, scale_r);
#else
retVec.data[0] = a.data[0] * scalar;
retVec.data[1] = a.data[1] * scalar;
retVec.data[2] = a.data[2] * scalar;
#endif
return retVec;

} /* vec3MultScalar */

/*******************************************************************************
* END OF FILE                                                                  *
*******************************************************************************/
