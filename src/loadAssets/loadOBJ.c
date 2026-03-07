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
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>

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

static void getMtlDiffuse(FILE* mtlFile, float retRGB[3])
{
// im gna do it the dumb way </3

int c;
while ((c = fgetc(mtlFile)) != EOF)
{
switch (c) 
    {
    case ' ':
    case '\t':
    case '\n':
        break;
    case 'K':
        if (fgetc(mtlFile) == 'd')
        {
        fscanf(mtlFile, " %f %f %f", retRGB, retRGB+1, retRGB+2);
        return;
        }
        break;
    default:
        break;
    }
}
}


static void getMaterialFromMtl(const char* mtlFileName, const char* mtlName, float retRGB[3]) 
{

char actualFileName[1024] = "../../../../emulator/resources/";

strcat(actualFileName, mtlFileName);

// TODO: strcat tjhis
FILE* mtlFile = fopen(actualFileName, "rb");

if ( mtlFile == NULL )
{
    printf("NOOOO!\n");
    exit(1);
}

char readMtlName[512];

int c;
while ((c = fgetc(mtlFile)) != EOF)
{
switch (c) 
    {
    case ' ':
    case '\t':
    case '\n':
        break;
    case '#':
        printf("Skipping Comment\n");
        skipToNextLine(mtlFile);
        break;
    case 'n':
        ungetc(c, mtlFile);
        fscanf(mtlFile, "%s", readMtlName);

        if ( strcmp(readMtlName, "newmtl") != 0 )
        {
        break;
        }
        fscanf(mtlFile, "%s", readMtlName);
        if ( strcmp(readMtlName, mtlName) != 0 )
        {
        break;
        }
        getMtlDiffuse(mtlFile, retRGB);
        printf("%f %f %f\n", *retRGB, *(retRGB+1), *(retRGB+2));
        fclose(mtlFile);
        return ;
        break;
    default:
        break;
    }

}

fclose(mtlFile);
}

static void parseVertex(FILE* file, float** vertexPositionData)
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
                DARRAY_PUSH_ESF(vertexPositionData, readFloat);
                //printf("VDATA: %f\n", readFloat);
                break;
        }
    }

}

static void parseFaceNormalIndex(FILE* file, struct meshObject* meshObject, const float* currentVertexNormalsData)
{
    // theres gonna be a float here, just read it bro
    int readInt;
    fscanf(file, "%d", &readInt);
    readInt--;
    DARRAY_PUSH(meshObject->vertexData, *(currentVertexNormalsData + readInt * 3));
    DARRAY_PUSH(meshObject->vertexData, *(currentVertexNormalsData + readInt * 3 + 1));
    DARRAY_PUSH(meshObject->vertexData, *(currentVertexNormalsData + readInt * 3 + 2));

    //printf("Read face normal index %f\n", readFloat);

}

static void parseFaceTextureIndex(FILE* file, struct meshObject* meshObject, const float* currentVertexNormalsData) 
{

// check to make sure this field is not empty
int c = fgetc(file);
switch (c) 
    {
        case '/':
            //printf("Texture index skipped\n");
            parseFaceNormalIndex(file, meshObject, currentVertexNormalsData);
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
    parseFaceNormalIndex(file, meshObject, currentVertexNormalsData);
    }
else
    {
    ungetc(c, file);
    }
}

static void parseFaceVertexIndex(FILE* file, struct meshObject* meshObject, const float* currentVertexPositionData, const float* currentVertexNormalsData, char lastChar) 
{

ungetc(lastChar, file);
int readInt;
fscanf(file, "%d", &readInt);
readInt--;
DARRAY_PUSH(meshObject->vertexData, *(currentVertexPositionData + readInt * 3));
DARRAY_PUSH(meshObject->vertexData, *(currentVertexPositionData + readInt * 3 + 1));
DARRAY_PUSH(meshObject->vertexData, *(currentVertexPositionData + readInt * 3 + 2));
//printf("FACE VINDEX: %d\n", readInt);
// add to array here
// Continue to next state if a / is found
int c = fgetc(file);
if (c == '/') 
    {
    parseFaceTextureIndex(file, meshObject, currentVertexNormalsData);
    }
else
    {
    ungetc(c, file);
    }
}


static void parseFace(FILE* file, struct meshObject* meshObject, const float* currentVertexPositionData, const float* currentVertexNormalsData, const float RGB[3]) 
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
                parseFaceVertexIndex(file, meshObject, currentVertexPositionData, currentVertexNormalsData, c);
                DARRAY_PUSH(meshObject->vertexData, RGB[0]);
                DARRAY_PUSH(meshObject->vertexData, RGB[1]);
                DARRAY_PUSH(meshObject->vertexData, RGB[2]);
                break;
        }
    }
}

static void parseVertexNormal(FILE* file, float** vertexNormalData)
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
                DARRAY_PUSH_ESF(vertexNormalData, readFloat);
                //printf("VDATA: %f\n", readFloat);
                break;
        }
    }

}


struct meshObject* loadVertexDataFromOBJ
    (
    const char* filepath
    ) 
{


/*
struct fileVertexData vData = 
    {
        .fileMaterialsData = DARRAY_NEW(struct fileMaterials, 10),
        .vertexData = DARRAY_NEW(float, 100),
        .vertexNormalsData = DARRAY_NEW(float, 100),
        .faceIndexData = DARRAY_NEW(unsigned int, 100),
        .vertexNormalsIndices = DARRAY_NEW(unsigned int, 100)
    };
*/


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
    return NULL;
    }

#endif

float* vertexPositionData = DARRAY_NEW(float, 100);
float* vertexNormalData = DARRAY_NEW(float, 100);

char mtlFileName[512];
char currMtlName[512];

float currMtlRGB[3];

struct meshObject* meshes = DARRAY_NEW(struct meshObject, 15);

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
            case 'o':
                int numC;
                struct meshObject thisMesh = 
                {
                    .objName = {},
                    .vertexData = DARRAY_NEW(float, 1000),
                };
                fscanf(srcFile, "%511s", &thisMesh.objName);

                DARRAY_PUSH(meshes, thisMesh);
                break;
            case 'v':
                if ((c = fgetc(srcFile)) == ' ')
                {
                parseVertex(srcFile, &vertexPositionData);
                } else if (c == 'n')
                {
                parseVertexNormal(srcFile, &vertexNormalData);
                }
                break; 
            case 'f':
                parseFace(srcFile, meshes + DARRAY_SIZE(meshes)-1, vertexPositionData, vertexNormalData, currMtlRGB);
                break;
            case 'm':
                ungetc(c, srcFile);
                fscanf(srcFile, "%s", mtlFileName);
                if ( strcmp("mtllib", mtlFileName) == 0 )
                {
                fscanf(srcFile, "%s", mtlFileName);
                printf("%s\n", mtlFileName);
                }
                break;
            case 'u':
                ungetc(c, srcFile);

                fscanf(srcFile, "%s", currMtlName);

                if (strcmp("usemtl", currMtlName) != 0) 
                {
                break;
                }

                fscanf(srcFile, "%s", currMtlName);

                getMaterialFromMtl(mtlFileName, currMtlName, currMtlRGB);

                printf("Using material %s\n", currMtlName);
                break; 
            default:
                break;

        }
    }

printf("\n");
fclose(srcFile);

return meshes;
}

/*******************************************************************************
* END OF FILE                                                                  *
*******************************************************************************/
