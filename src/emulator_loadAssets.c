#include <stddef.h>
#include <string.h>
#define __STDC_WANT_LIB_EXT1__ 1
#include <stdlib.h>
#include <stdio.h>
#include "emulator_loadAssets.h"

static void skipToNextLine(FILE* file) 
{
fscanf(file, "%*[^\n]");
}

static void addVertexToFileStructArray(struct fileVertexData* fileVertexData, float val) 
{

if ( fileVertexData->vertexDataCount + 1 > fileVertexData->vertexDataSize ) 
    {
    printf("Reized vertex data\n");
    fileVertexData->vertexDataSize *= 2;
    fileVertexData->vertexData = realloc(fileVertexData->vertexData, sizeof(float) * fileVertexData->vertexDataSize);
    }

printf("Updated vertex data\n");
*(fileVertexData->vertexData + fileVertexData->vertexDataCount) = val;
fileVertexData->vertexDataCount++;

}

static void addFaceVertexIndexToFileStructArray(struct fileVertexData* fileVertexData, int val)
{
    // These indices are 1-based, so offset by one
val--;
if ( fileVertexData->faceIndexDataCount + 1 > fileVertexData->faceIndexDataSize) 
    {
    printf("Reized face vertex index data\n");
    fileVertexData->faceIndexDataSize *= 2;
    fileVertexData->faceIndexData = realloc(fileVertexData->faceIndexData, sizeof(int) * fileVertexData->vertexDataSize);
    }

printf("Updated face vertex index data\n");
*(fileVertexData->faceIndexData + fileVertexData->faceIndexDataCount) = val;
fileVertexData->faceIndexDataCount++;

}

static void parseVertex(FILE* file, struct fileVertexData* fileVertexData)
{

int c;
// Hopefully this covers all platforms? although I think I'm the odd one since windows lol
while ((c = fgetc(file)) != '\n' && c != '\r') 
    {
        printf("PARSE VERTEX C: %c\n", c);
        switch (c) 
        {
            case ' ':
            case '\t':
                // ignore
                break;
            default:
                // Anything else implies there is going to be a float next, go back 1 char and read the float
                ungetc(c, file);
                float readFloat;
                fscanf(file, "%f", &readFloat);
                addVertexToFileStructArray(fileVertexData, readFloat);
                printf("VDATA: %f\n", readFloat);
                break;
        }
    }

}

static void parseFaceNormalIndex(FILE* file, struct fileVertexData* fileVertexData)
{
    // theres gonna be a float here, just read it bro
    float readFloat;
    fscanf(file, "%f", &readFloat);
    printf("Read face normal index %f\n", readFloat);

}

static void parseFaceTextureIndex(FILE* file, struct fileVertexData* fileVertexData) 
{

// check to make sure this field is not empty
int c = fgetc(file);
switch (c) 
    {
        case '/':
            printf("Texture index skipped\n");
            parseFaceNormalIndex(file, fileVertexData);
            break;
        default:
            ungetc(c, file);
            // read the float (does nothing rn)
            int readInt;
            fscanf(file, "%d", &readInt);
            printf("Reads texture index: %d\n", readInt);
            break;
    }

c = fgetc(file);
if ( c == '/' )
    {
    parseFaceNormalIndex(file, fileVertexData);
    }
else
    {
    ungetc(c, file);
    }
}

static void parseFaceVertexIndex(FILE* file, struct fileVertexData* fileVertexData, int lastChar) 
{

ungetc(lastChar, file);
int readInt;
fscanf(file, "%d", &readInt);
printf("FACE VINDEX: %d\n", readInt);
// add to array here
addFaceVertexIndexToFileStructArray(fileVertexData, readInt);
// Continue to next state if a / is found
int c = fgetc(file);
if (c == '/') 
    {
    parseFaceTextureIndex(file, fileVertexData);
    }
else
    {
    ungetc(c, file);
    }
}


static void parseFace(FILE* file, struct fileVertexData* fileVertexData) 
{
int c;
while ((c = fgetc(file)) != '\n' && c != '\r') 
    {
        printf("PARSE FACE C: %c\n", c);
        switch (c) 
        {
            case ' ':
            case '\t':
                break;
            case '/':
                // found the 
                break;
            default:
                // Anything else implies a float next, go back 1 char and read
                parseFaceVertexIndex(file, fileVertexData, c);
                break;
        }
    }
}


struct fileVertexData loadVertexDataFromOBJ
    (
    const char* filepath
    ) 
{


struct fileVertexData vData = 
    {
        .vertexDataSize = 100,
        .vertexDataCount= 0,
        .vertexData = malloc(sizeof(float) * vData.vertexDataSize),

        .faceIndexDataSize = 100,
        .faceIndexDataCount = 0,
        .faceIndexData = malloc(sizeof(int) * vData.faceIndexDataSize)
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

int c;
while ((c = fgetc(srcFile)) != EOF)
    {
    switch (c) 
        {
            case ' ':
            case '\t':
            case '\n':
                break;
            case '#':
                printf("Skipping Comment\n");
                skipToNextLine(srcFile);
                break;
            case 'v':
                parseVertex(srcFile, &vData);
                break; 
            case 'f':
                parseFace(srcFile, &vData);
                break;
            default:
                break;

        }
    }

printf("\n");
fclose(srcFile);

// trim
printf("Trimmed vertex data to size %lu floats (%lu bytes)\n", vData.vertexDataCount, sizeof(float) * vData.vertexDataCount);
vData.vertexData = realloc(vData.vertexData, sizeof(float) * vData.vertexDataCount);
vData.vertexDataSize = vData.vertexDataCount;
printf("Trimmed face vertex index data to size %lu ints (%lu bytes)\n", vData.faceIndexDataCount, sizeof(int) * vData.faceIndexDataCount);
vData.faceIndexData = realloc(vData.faceIndexData, sizeof(int) * vData.faceIndexDataCount);
vData.faceIndexDataSize = vData.faceIndexDataCount;
return vData;
}
