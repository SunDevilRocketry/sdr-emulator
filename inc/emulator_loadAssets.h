#ifndef EMULATOR_LOAD_ASSETS_H_
#define EMULATOR_LOAD_ASSETS_H_
#include <stddef.h>

struct fileVertexData {
    size_t vertexDataSize; // Used in parsing to contain full size of array;same as data count otherwise 
    size_t vertexDataCount;
    float *vertexData;

    size_t faceIndexDataSize; // Used in parsing to contain full size of array; same as data count otherwise
    size_t faceIndexDataCount;
    unsigned int *faceIndexData;
};

struct fileVertexData loadVertexDataFromOBJ(const char* filepath);

#endif // EMULATOR_LOAD_ASSETS_H_
