#ifndef MAT4_H_
#define MAT4_H_

// Column major

typedef union vec3 {
    float data[3];
    // Anonymous structs are a microsoft extension :(
    struct _vec3_members {
        float x;
        float y;
        float z;
    } members;
} vec3;

// typedef?
typedef union mat4 {
    float data[16];
    // Anonymous structs are a microsoft extension :(
    struct _mat4_members {
        // a[row][col]
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

vec3 vec3New(const float x, const float y, const float z);
float vec3Dot(const vec3 a, const vec3 b);
vec3 vec3Cross(const vec3 a, const vec3 b);
float vec3Magnitude(const vec3 vec);
vec3 vec3Normalize(const vec3 vec);
vec3 vec3Sub(const vec3 a, const vec3 b);
vec3 vec3MultScalar(const vec3 a, float scalar);

mat4 mat4Identity();
mat4 mat4Mult(const mat4 a, const mat4 b);
mat4 mat4MultScalar(const mat4 a, float scalar);
mat4 mat4Add(const mat4 a, const mat4 b);
mat4 mat4RotY(float angle);
mat4 mat4AxisAngle(vec3 axis, float angle);
mat4 mat4Translation(float x, float y, float z);
mat4 mat4LookAt(vec3 position, vec3 target, vec3 up);
mat4 mat4Proj
    (
    float fov,
    float aspect,
    float near,
    float far
    );

// IDK if ortho is needed
mat4 mat4Ortho(float x, float y, float z);

void mat4_debugprint(mat4 mat);
void vec3_debugprint(vec3 vec);

#endif // MAT4_H_
