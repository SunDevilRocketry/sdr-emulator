#include "math/lin.h"
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

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
}

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
}
/* Performs a X b */
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

}

float vec3Magnitude
    (
    const vec3 vec
    )
{
    return sqrt(pow(vec.data[0], 2) + pow(vec.data[1], 2) + pow(vec.data[2], 2));
}

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
}

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
}

vec3 vec3MultScalar(const vec3 a, float scalar) 
{
vec3 retVec;
retVec.data[0] = a.data[0] * scalar;
retVec.data[1] = a.data[1] * scalar;
retVec.data[2] = a.data[2] * scalar;
return retVec;
}

void vec3_debugprint
    (
    vec3 vec
    )
{

printf("VEC3 (ARRAY ELEMENTS): (%.4f, %.4f, %.4f)\n", vec.data[0], vec.data[1], vec.data[2]);
printf("VEC3 (MEMBERS): (%.4f, %.4f, %.4f)\n", vec.members.x, vec.members.y, vec.members.z);

}
