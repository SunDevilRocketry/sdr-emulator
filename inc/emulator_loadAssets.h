#ifndef EMULATOR_LOAD_ASSETS_H_
#define EMULATOR_LOAD_ASSETS_H_
#include <stddef.h>

struct fileVertexData {
    size_t vFloatCount;
    float *vertexData;

    size_t iCount;
    unsigned int *faceIndexData;
};

struct fileVertexData loadVertexDataFromOBJ(const char* filepath);

#endif // EMULATOR_LOAD_ASSETS_H_
