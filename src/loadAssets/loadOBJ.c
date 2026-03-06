/*******************************************************************************
*
* FILE: 
* 		loadOBJ.c
*
* DESCRIPTION: 
* 		''''State machine'''' implementation that parses OBJ files and returns
* 		a fileVertexData struct describing the OBJ data.
*                                                                             
* COPYRIGHT:                                                                  
*       Copyright (c) 2026 Sun Devil Rocketry.                                
*       All rights reserved.                                                  
*                                                                             
*       This software is licensed under terms that can be found in the LICENSE
*       file in the root directory of this software component.                 
*       If no LICENSE file comes with this software, it is covered under the   
*       BSD-3-Clause.                                                          
*                                                                              
*       https://opensource.org/license/bsd-3-clause                            
*
*******************************************************************************/

/*------------------------------------------------------------------------------
 Standard Includes                                                                    
------------------------------------------------------------------------------*/
#define __STDC_WANT_LIB_EXT1__ 1
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/*------------------------------------------------------------------------------
 Project Includes
------------------------------------------------------------------------------*/
#include "containers/darr.h"
#include "loadAssets/loadAssets.h"
/* TODO: The renderer still does not draw the test geometry correctly. I have no idea why, but it draws the REV2 just fine. ugh. fix later */

/*------------------------------------------------------------------------------
 Static Function Prototypes
------------------------------------------------------------------------------*/
static void skipToNextLine
    (
    FILE* file
    );

/*------------------------------------------------------------------------------
 Functions 
------------------------------------------------------------------------------*/
static void skipToNextLine(FILE* file) 
{
fscanf(file, "%*[^\n]");
}

static void addVertexToFileStructArray(struct fileVertexData* fileVertexData, float val) 
{
       
(void)DARRAY_PUSH(fileVertexData->vertexData, val);

}

static void addFaceVertexIndexToFileStructArray(struct fileVertexData* fileVertexData, unsigned int val)
{
    // These indices are 1-based, so offset by one
val--;
(void)DARRAY_PUSH(fileVertexData->faceIndexData, val);

}

static void addVertexNormalToFileStructArray(struct fileVertexData* fileVertexData, float val)
{

(void)DARRAY_PUSH(fileVertexData->vertexNormalsData, val);

}

static void addFaceNormalIndexToFilestructArray(struct fileVertexData* fileVertexData, unsigned int val)
{
val--;
(void)DARRAY_PUSH(fileVertexData->vertexNormalsIndices, val);
}

static void parseVertex(FILE* file, struct fileVertexData* fileVertexData)
{

int c;
// Hopefully this covers all platforms? although I think I'm the odd one since windows lol
while ((c = fgetc(file)) != '\n' && c != '\r') 
    {
        //printf("PARSE VERTEX C: %c\n", c);
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
                //printf("VDATA: %f\n", readFloat);
                break;
        }
    }

}

static void parseFaceNormalIndex(FILE* file, struct fileVertexData* fileVertexData)
{
    // theres gonna be a float here, just read it bro
    unsigned int readIndex;
    fscanf(file, "%d", &readIndex);
    addFaceNormalIndexToFilestructArray(fileVertexData, readIndex);

    //printf("Read face normal index %f\n", readFloat);

}

static void parseFaceTextureIndex(FILE* file, struct fileVertexData* fileVertexData) 
{

// check to make sure this field is not empty
int c = fgetc(file);
switch (c) 
    {
        case '/':
            //printf("Texture index skipped\n");
            parseFaceNormalIndex(file, fileVertexData);
            break;
        default:
            ungetc(c, file);
            // read the float (does nothing rn)
            int readInt;
            fscanf(file, "%d", &readInt);
            //printf("Reads texture index: %d\n", readInt);
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
//printf("FACE VINDEX: %d\n", readInt);
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
        //printf("PARSE FACE C: %c\n", c);
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

static void parseVertexNormal(FILE* file, struct fileVertexData* fileVertexData)
{
int c;
while ((c = fgetc(file)) != '\n' && c != '\r') 
    {
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
                addVertexNormalToFileStructArray(fileVertexData, readFloat);
                //printf("VDATA: %f\n", readFloat);
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
        .vertexData = DARRAY_NEW(float, 100),
        .vertexNormalsData = DARRAY_NEW(float, 100),
        .faceIndexData = DARRAY_NEW(unsigned int, 100),
        .vertexNormalsIndices = DARRAY_NEW(unsigned int, 100)
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
                if ((c = fgetc(srcFile)) == ' ')
                {
                parseVertex(srcFile, &vData);
                } else if (c == 'n')
                {
                parseVertexNormal(srcFile, &vData);
                }
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

return vData;
}

/*******************************************************************************
* END OF FILE                                                                  *
*******************************************************************************/
