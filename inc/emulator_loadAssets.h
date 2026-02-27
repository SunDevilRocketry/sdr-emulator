#ifndef EMULATOR_LOAD_ASSETS_H_
#define EMULATOR_LOAD_ASSETS_H_
#include <stddef.h>

struct fileVertexData {
    size_t  vertexCount;
    size_t vertexCapacity;
    float* vertices;
};

struct fileVertexData loadVertexDataFromOBJ(const char* filepath);

#endif // EMULATOR_LOAD_ASSETS_H_
