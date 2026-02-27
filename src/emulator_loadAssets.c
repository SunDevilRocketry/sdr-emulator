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
        .vertices = NULL
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

size_t vertexCount = 0;
size_t vertexCapacity = 50;

// Init size to 50
vData.vertices = malloc(sizeof(float) * vertexCapacity);


while ( fscanf(srcFile, "%s", elementSpecifier) != EOF ) 
    {

        if ( strcmp(elementSpecifier, "v" ) == 0 )
            {
//            printf("Element specifier: %s\n", elementSpecifier);
 //           printf("VERTEX DETECTED\n");
            float vertices[3];
            fscanf(srcFile, "%f %f %f", &vertices[0], &vertices[1], &vertices[2]);
  //              printf("VERTEX COORDINATES: %.2f, %.2f, %.2f\n", vertices[0], vertices[1], vertices[2]);
            if ( vertexCount + 3 < vertexCapacity ) 
                {
  //              printf("Copying to vertex array\n");
                memcpy(vData.vertices + vertexCount, vertices, sizeof(float) * 3);
                vertexCount += 3;
                }
            else
                {
                vertexCapacity *= 1.5;
                vData.vertices = realloc(vData.vertices, sizeof(float) * vertexCapacity);
                printf("Expanding vertex array to %lu and copying\n", vertexCapacity);
                memcpy(vData.vertices + vertexCount, vertices, sizeof(float) * 3);
                vertexCount += 3;
                }
            }

    }

fclose(srcFile);

vData.vertexCount = vertexCount;
vData.vertexCapacity = vertexCapacity;

printf("thingo");
return vData;
}
