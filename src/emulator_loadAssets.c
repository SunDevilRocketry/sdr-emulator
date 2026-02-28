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

// 10 is probably enough, they typically are only 2 chars at max
char elementSpecifier[10];

size_t vFloatCount= 0;
size_t vFloatCapacity = 100;

size_t iCount = 0;
size_t iCapacity = 100;


// Init size to initial capacity
vData.vertexData = malloc(sizeof(float) * vFloatCapacity);
vData.faceIndexData = malloc(sizeof(float) * iCapacity);

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
        else if ( strcmp(elementSpecifier, "f") == 0 )
            {
            // This indicates a face index specifier for EBO    
            // There can be any amount of vertices for each face
            // There are also negative indices but im ignoring those because i dont wanna deal with them rn
            // 1 Figure out how many chars until newline
            // 2 Scanf until I pass that char limit
            // 3 undo last scanf and break loop
            
            // Since this file is open in binary mode, arithmetic with ftell return value is well defined

            long int initialPos = ftell(srcFile);
            // Skips everything up to the newline
            fscanf(srcFile, "%*[^\n]");
            long int finalPos = ftell(srcFile);

            long int dataLength = finalPos - initialPos;
            //printf("%ld chars to read\n", dataLength);

            long int charsRead = 0;

            fseek(srcFile, initialPos, SEEK_SET);
            while (charsRead < dataLength) {
                int dummy1;
                int dummy2;
                int vIndex;

                long int beforeScanPos = ftell(srcFile);
                fscanf(srcFile, "%d/%d/%d", &vIndex, &dummy1, &dummy2);
                long int currentPos = ftell(srcFile);
                charsRead = currentPos - initialPos;
                if (charsRead < dataLength) 
                {
                    // Insert into array
                    //printf("vIndex: %d\n", vIndex);
                    if (iCount + 1 < iCapacity) 
                    {
                    vIndex--;
                    memcpy(vData.faceIndexData + iCount, &vIndex, sizeof(int));
                    iCount++;
                    }
                    else
                    {
                    iCapacity *= 2;
                    printf("Resizing face index data to %ld (%ld bytes)\n", iCapacity, sizeof(int) * iCapacity);
                    vData.faceIndexData = realloc(vData.faceIndexData, sizeof(int) * iCapacity);

                    vIndex--;
                    memcpy(vData.faceIndexData + iCount, &vIndex, sizeof(int));
                    iCount++;
                    }
                }
                else 
                {
                    //printf("TOO FAR!\n");
                    fseek(srcFile, beforeScanPos, SEEK_SET);
                    break;
                }
                //printf("Total chars read: %ld\n", charsRead);
            }

            }

    }

fclose(srcFile);

// Trim buffer to free excess space
printf("Trimming vertex data buffer to %ld floats (%ld bytes)\n", vFloatCount, sizeof(float) * vFloatCount);
vData.vertexData = realloc(vData.vertexData, sizeof(float) * vFloatCount);

printf("Trimming index data buffer to %ld floats (%ld bytes)\n", iCount, sizeof(float) * iCount);
vData.vertexData = realloc(vData.vertexData, sizeof(int) * iCount);

vData.vFloatCount = vFloatCount;
vData.iCount = iCount;

printf("thingo");
return vData;
}
