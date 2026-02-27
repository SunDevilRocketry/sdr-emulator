#include <stddef.h>
#include <string.h>
#define __STDC_WANT_LIB_EXT1__ 1
#include <stdlib.h>
#include <stdio.h>
#include "emulator_loadAssets.h"


struct fileVertexData loadVertexDataFromOBJ
    (
    const char* filepath
    ) 
{


struct fileVertexData vData = 
    {
        .vFloatCount = 0,
        .vertexData = NULL
    };

FILE *srcFile = NULL;

#ifdef __STDC_LIB_EXT1__

errno_t fopenError = fopen_s(&srcFile, filepath, "rb");

if ( fopenError != 0 ) 
    {
    printf("[SECURE FOPEN]: Failed to open %s, error %d\n", filepath, fopenError);
    return vData;
    }

#else

srcFile = fopen(filepath, "rb");

if ( srcFile == NULL ) 
    {
        printf("[NON-SECURE FOPEN]: Failed to open %s\n", filepath);
    return vData;
    }

#endif

// 10 is probably enough
char elementSpecifier[10];

size_t vFloatCount= 0;
size_t vFloatCapacity = 100;

// Init size to initial capacity
vData.vertexData = malloc(sizeof(float) * vFloatCapacity);


while ( fscanf(srcFile, "%s", elementSpecifier) != EOF ) 
    {

        if ( strcmp(elementSpecifier, "v" ) == 0 )
            {
//            printf("Element specifier: %s\n", elementSpecifier);
 //           printf("VERTEX DETECTED\n");
            float vertices[3];
            fscanf(srcFile, "%f %f %f", &vertices[0], &vertices[1], &vertices[2]);
  //              printf("VERTEX COORDINATES: %.2f, %.2f, %.2f\n", vertices[0], vertices[1], vertices[2]);
            if ( vFloatCount + 3 < vFloatCapacity ) 
                {
  //              printf("Copying to vertex array\n");
                memcpy(vData.vertexData + vFloatCount, vertices, sizeof(float) * 3);
                vFloatCount += 3;
                }
            else
                {
                vFloatCapacity *= 2;
                vData.vertexData = realloc(vData.vertexData, sizeof(float) * vFloatCapacity);
                printf("Expanding vertex array to %lu and copying\n", vFloatCapacity);
                memcpy(vData.vertexData + vFloatCount, vertices, sizeof(float) * 3);
                vFloatCount += 3;
                }
            }

    }

fclose(srcFile);

// Trim buffer to free excess space
printf("Trimming vertex data buffer to %ld floats (%ld bytes)\n", vFloatCount, sizeof(float) * vFloatCount);
vData.vertexData = realloc(vData.vertexData, sizeof(float) * vFloatCount);

vData.vFloatCount = vFloatCount;

printf("thingo");
return vData;
}
