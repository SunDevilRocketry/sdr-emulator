#version 430 core

layout (location = 0) in vec3 pos;
layout (location = 1) in vec3 vertexNormal;
layout (location = 2) in vec3 vertexColor;

out vec3 fragPos;
out vec3 normal;
out vec3 color;

uniform mat4 model;
uniform mat4 view;
uniform mat4 proj;

void main() {
    gl_Position = proj * view * model * vec4(pos, 1.0);

    fragPos = vec3(model * vec4(pos, 1.0));
    //normal = mat3(transpose(inverse(model))) * vertexNormal; // Note: This is dumb

    normal = mat3(transpose(inverse(model))) * vertexNormal;
    color = vertexColor;
}
