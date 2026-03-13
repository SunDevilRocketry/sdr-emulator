/*******************************************************************************
*
* FILE: 
* 		lin.h
*
* DESCRIPTION: 
*       Provides types for passing coordinate/transform data to shaders.
*       Provides function prototypes for manipulating coordinates and transforms.
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

#ifndef LIN_H_ 
#define LIN_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifdef GRAPHICS_DEBUG
#define GRAPHICS_OPTIMIZED_PROCEDURE __attribute__((optimize("Og")))
#else
#define GRAPHICS_OPTIMIZED_PROCEDURE __attribute__((optimize("O3")))
#endif

/*------------------------------------------------------------------------------
 Typedefs
------------------------------------------------------------------------------*/

typedef union vec3 {
    float data[3];
    struct _vec3_members {
        float x;
        float y;
        float z;
    } members;
} vec3;

_Static_assert( sizeof(vec3) == 12, "vec3s should only contain 3 floats" );

/* Matrices are stored in column major order per the GLSL specification. */
/* This has no effect on their operations or use */
typedef union mat4 {
    float data[16];
    struct _mat4_members {
        /* a[row][col] */
        float a00;
        float a01;
        float a02;
        float a03;
        float a10;
        float a11;
        float a12;
        float a13;
        float a20;
        float a21;
        float a22;
        float a23;
        float a30;
        float a31;
        float a32;
        float a33;
    } members;
} mat4;

_Static_assert( sizeof(mat4) == 64, "mat4s should only contain 16 floats" );

/*------------------------------------------------------------------------------
 Function prototypes                                             
------------------------------------------------------------------------------*/

/* VEC3 FUNCTIONS */
/* vec3.c */

vec3 vec3New
    (
    float x, 
    float y, 
    float z
    );


vec3 vec3Cross
    (
    vec3 a, 
    vec3 b
    );


vec3 vec3Normalize
    (
    vec3 vec
    );

vec3 vec3Sub
    (
    vec3 a, 
    vec3 b
    );

vec3 vec3MultScalar
    (
    vec3 a, 
    float scalar
    );

float vec3Magnitude
    (
    const vec3 vec
    );

float vec3Dot
    (
    vec3 a, 
    vec3 b
    );

/* MAT4 FUNCTIONS */
/* mat4.c */

mat4 mat4Identity
    (
    void
    );

mat4 mat4Mult
    (
    mat4 a, 
    mat4 b
    );

mat4 mat4MultScalar
    (
    mat4 a, 
    float scalar
    );
    
mat4 mat4Add
    (
    mat4 a, 
    mat4 b
    );

mat4 mat4RotY
    (
    float angle
    );

mat4 mat4AxisAngle
    (
    vec3 axis, 
    float angle
    );

mat4 mat4Translation
    (
    float x, 
    float y, 
    float z
    );

mat4 mat4LookAt
    (
    vec3 position, 
    vec3 target, 
    vec3 up
    );

mat4 mat4Proj
    (
    float fov,
    float aspect,
    float near,
    float far
    );

#ifdef __cplusplus
}
#endif

#endif /* LIN_H_ */

/*******************************************************************************
* END OF FILE                                                                  *
*******************************************************************************/

