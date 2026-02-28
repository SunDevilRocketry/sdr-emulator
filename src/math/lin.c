#include "math/lin.h"
#include <math.h>
#include <stdint.h>
#include <stdio.h>

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

mat4 mat4Identity()
{

mat4 retMat4 = {};
for (size_t col = 0; col < 4; col++) {
    for (size_t row = 0; row < 4; row++) {
        *(retMat4.data + col * 4 + row) = (col == row) ? 1 : 0;
    }
}

return retMat4;

}

mat4 mat4Mult(const mat4 a, const mat4 b);

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
}
mat4 mat4LookAt
    (
    vec3 position, 
    vec3 target,
    vec3 up
    ) 
{

// Camera looks down the -Z axis, so this finds the negative look vector
vec3 zAxis = vec3Normalize(vec3Sub(position, target));
vec3 xAxis = vec3Normalize(vec3Cross(up, zAxis));
vec3 cameraUp = vec3Normalize(vec3Cross(zAxis, xAxis));
// Do note that this process has a side effect of causing issues when the up vector is the same as the zAxis (-lookDirection)

mat4 lookAt = mat4Identity(); 
// First column, X_1, Y_1, Z_1
lookAt.data[0] = xAxis.members.x;
lookAt.data[1] = cameraUp.members.x;
lookAt.data[2] = zAxis.members.x;
// Second column, X_2, Y_2, Z_2
lookAt.data[4] = xAxis.members.y;
lookAt.data[5] = cameraUp.members.y;
lookAt.data[6] = zAxis.members.y;
// Third column, X_3, Y_3, Z_3
lookAt.data[8] = xAxis.members.z;
lookAt.data[9] = cameraUp.members.z;
lookAt.data[10] = zAxis.members.z;
// Fourth column, -Px, -Py, -Pz (instead of moving the 'camera' by some vector, we just move everything else by the negation of that vector)
lookAt.data[12] = -position.members.x;
lookAt.data[13] = -position.members.y;
lookAt.data[14] = -position.members.z;

return lookAt;
}

mat4 mat4Proj
    (
    float fov,
    float aspect,
    float near,
    float far
    ) 
{

// Goal: We want to define a global coordinate space that our camera can 'look' into and render a result to the screen.
// Problem 1: OpenGL Only renders vertices that are within the NDC range ([-1, 1] for x and y), so most of our global world would not be visible.
// Problem 2: We want things to look realistic. Vertices that are further away should be smaller.

/* Solution: Interpret the visible world as a frustrum. Project all vertices in the frustrum onto the near plane. This allows all resulting coordinate to be normalized to [-1, 1] and takes care of making further vertices smaller via perspective divide.*/


float xMult = 1/(aspect*tan(fov/2.0));
float yMult = 1/tan(fov/2.0);
float zMult = -((far + near)/(far - near));

mat4 proj = {};
proj.data[0] = xMult;
proj.data[5] = yMult;
proj.data[10] = zMult;
proj.data[11] = -1;
// Dont really understand the z component modifications. Its some kind of non-linear mapping
proj.data[14] = -(2*far*near)/(far-near);

return proj;
}

// IDK if ortho is needed
mat4 mat4Ortho(float x, float y, float z);




void mat4_debugprint
    (
    mat4 mat
    )
{

for (int row = 0; row < 4; row++) {
    for (int col = 0; col < 4; col++) {
        printf("%.2f ", *(mat.data + row + 4 * col));
    }
    printf("\n");
}

}

void vec3_debugprint
    (
    vec3 vec
    )
{

printf("VEC3 (ARRAY ELEMENTS): (%.4f, %.4f, %.4f)\n", vec.data[0], vec.data[1], vec.data[2]);
printf("VEC3 (MEMBERS): (%.4f, %.4f, %.4f)\n", vec.members.x, vec.members.y, vec.members.z);

}
