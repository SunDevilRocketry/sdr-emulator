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

static void getMtlDiffuse(FILE* mtlFile, struct fileMaterials* material)
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
        fscanf(mtlFile, " %f %f %f", &material->r, &material->g, &material->b);
        return;
        }
        break;
    default:
        break;
    }
}
}


static struct fileMaterials getMaterialFromMtl(const char* mtlFileName, const char* mtlName) 
{
struct fileMaterials material = {};

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
        getMtlDiffuse(mtlFile, &material);
        printf("%f %f %f\n", material.r, material.g, material.b);
        fclose(mtlFile);
        return material;
    break;
    default:
        break;
    }

}

fclose(mtlFile);
return material;
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


static int parseFace(FILE* file, struct fileVertexData* fileVertexData) 
{
int numIndicesParsed = 0;
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
                numIndicesParsed++;
                break;
        }
    }
return numIndicesParsed;
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
        .fileMaterialsData = DARRAY_NEW(struct fileMaterials, 10),
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

char mtlFileName[512];
char currMtlName[512];
char lastMtlName[512];
ssize_t numIndicesWithCurrMtl = -1;

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
                numIndicesWithCurrMtl+=parseFace(srcFile, &vData);
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

                // ITS SKIPPING MATS!!!
                fscanf(srcFile, "%s", currMtlName);

                if (numIndicesWithCurrMtl != -1) 
                {
                // put in mat data arr
                struct fileMaterials mat = getMaterialFromMtl(mtlFileName, lastMtlName);
                mat.numIndiciesUsingMat = numIndicesWithCurrMtl;
                DARRAY_PUSH(vData.fileMaterialsData, mat);
                } 

                strcpy(lastMtlName, currMtlName);

                numIndicesWithCurrMtl = 0;
                printf("Using material %s\n", currMtlName);
                break; 
            default:
                break;

        }
    }

printf("\n");
fclose(srcFile);

struct fileMaterials mat = getMaterialFromMtl(mtlFileName, currMtlName);
mat.numIndiciesUsingMat = numIndicesWithCurrMtl;
DARRAY_PUSH(vData.fileMaterialsData, mat);

return vData;
}

/*******************************************************************************
* END OF FILE                                                                  *
*******************************************************************************/
