/**
 * @file loadOBJ.c
 *
 * ''''State machine'''' implementation that parses OBJ files and returns
 * a mesh struct array describing the obj file.
 *
 * @note This parser expects obj file data to be triangulated
 *
 * @copyright
 *       Copyright (c) 2026 Sun Devil Rocketry.                                
 *       All rights reserved.                                                  
 *                                                                             
 *       This software is licensed under terms that can be found in the LICENSE
 *       file in the root directory of this software component.                 
 *       If no LICENSE file comes with this software, it is covered under the   
 *       BSD-3-Clause.                                                          
 *                                                                              
 *       https://opensource.org/license/bsd-3-clause                            
 */

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

/*------------------------------------------------------------------------------
 Static Function Prototypes
------------------------------------------------------------------------------*/
static void skipToNextLine
    (
    FILE* file
    );

static void getMtlDiffuse
    (
    FILE* mtlFile, 
    float retRGB[3]
    );

static void getMaterialFromMtl
    (
    const char* mtlFileName, 
    const char* mtlName, 
    float retRGB[3]
    );

static void parseVertex
    (
    FILE* file, 
    float** vertexPositionData
    );

static void parseFaceNormalIndex
    (
    FILE* file, 
    struct meshObject* meshObject, 
    const float* currentVertexNormalsData
    );

static void parseFaceTextureIndex
    (
    FILE* file, 
    struct meshObject* meshObject, 
    const float* currentVertexNormalsData
    );

static void parseFaceVertexIndex
    (
    FILE* file, 
    struct meshObject* meshObject, 
    const float* currentVertexPositionData, 
    const float* currentVertexNormalsData
    );

static void parseFace
    (
    FILE* file, 
    struct meshObject* meshObject, 
    const float* currentVertexPositionData, 
    const float* currentVertexNormalsData, 
    const float RGB[3]
    );

static void parseVertexNormal
    (
    FILE* file, 
    float** vertexNormalData
    );

/*------------------------------------------------------------------------------
 Functions 
------------------------------------------------------------------------------*/

/**
 * @brief Returns a DARRAY of mesh objects read from the obj 
 *
 * @param filepath The filepath to the obj from which to read
 *
 * @return Array of struct meshObjects describing objects in the file
 *
 * @note The OBJ's MTL file should exist in the same directory as the OBJ
 * @warning Do not reference a mtl file from the obj which does not exist 💀
 */
struct meshObject* loadVertexDataFromOBJ
    (
    const char* filepath
    ) 
{

/* Open obj file for reading */
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

/* Initialize data needed for parsing */

float* vertexPositionData = DARRAY_NEW(float, 100);
float* vertexNormalData = DARRAY_NEW(float, 100);

char mtlFileName[512];
char currMtlName[512];

float currMtlRGB[3];

struct meshObject* meshes = DARRAY_NEW(struct meshObject, 15);

/* Read file char by char until EOF */
int c;
while ((c = fgetc(srcFile)) != EOF)
    {
    switch (c) 
        {
        case ' ':
        case '\t':
        case '\n':
            /* Ignore whitespace */
            break;
        case '#':
            /* Ignore comments */
            skipToNextLine(srcFile);
            break;
        case 'o':
            /* 'o' indicates an object definition, push a new object onto the meshes DARRAY */
            struct meshObject thisMesh = 
            {
                .objName = {},
                .vertexData = DARRAY_NEW(float, 1000),
            };
            /* Scan object name */
            fscanf(srcFile, "%511s", thisMesh.objName);

            (void)DARRAY_PUSH(meshes, thisMesh);
            break;
        case 'v':
            /* v indicates some vertex attribute */
            if ((c = fgetc(srcFile)) == ' ')
            {
            /* If there is no next letter, a vertex position is defined */
            parseVertex(srcFile, &vertexPositionData);
            } else if (c == 'n')
            {
            /* If next letter is a n, a vertex normal is defined */
            parseVertexNormal(srcFile, &vertexNormalData);
            }
            break; 
        case 'f':
            /* f indicates a face will be defined, read the face data to the mesh at the end of the meshes DARRAY */
            parseFace(srcFile, meshes + DARRAY_SIZE(meshes)-1, vertexPositionData, vertexNormalData, currMtlRGB);
            break;
        case 'm':
            /* m indicates the definition of a linked .mtl file for this obj file */
            /* Put that m back and ensure the statement is mtllib */
            ungetc(c, srcFile);
            fscanf(srcFile, "%511s", mtlFileName);

            if ( strcmp("mtllib", mtlFileName) != 0 )
            {
            break;
            }

            /* Read the mtlFileName and store it for use when reading usemtl information */
            fscanf(srcFile, "%511s", mtlFileName);
            break;
        case 'u':
            /* u indicates a potential usemtl statement */
            /* Put that u back and ensure usemtl is in fact the statement */
            ungetc(c, srcFile);
            fscanf(srcFile, "%511s", currMtlName);

            if (strcmp("usemtl", currMtlName) != 0) 
            {
            break;
            }

            /* Read name of material */
            fscanf(srcFile, "%511s", currMtlName);
            
            /* Put material diffuse into the currMtlRGB */
            getMaterialFromMtl(mtlFileName, currMtlName, currMtlRGB);

            break; 
        default:
            break;

        }
    }
DARRAY_FREE(vertexPositionData);
DARRAY_FREE(vertexNormalData);
fclose(srcFile);

return meshes;
} /* loadVertexDataFromOBJ */

/*------------------------------------------------------------------------------
 Static Functions 
------------------------------------------------------------------------------*/

/**
 * Reads from file until the next newline, effectively skipping one line
 *
 * @param file The file stream to skip to next line
 */
static void skipToNextLine
    (
    FILE* 
    file) 
{
fscanf(file, "%*[^\n]");
}

/**
 * Reads the diffuse material value in a material block within the passed mtlFile.
 * The read diffuse value is stored in the three component array passed to the function.
 *
 * @param mtlFile The mtl file stream from which to read the diffuse value
 * @param retRGB The array to populate with the read diffuse values
 *
 * @note This function is intended to be called when the file stream has already been pushed up to the correct material block
 */
static void getMtlDiffuse
    (
    FILE* mtlFile, 
    float retRGB[3]
    )
{
/* im gna do it the dumb way </3 */
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
} /* getMtlDiffuse */


/**
 * Gets the diffuse material value from the material in an mtlFile and returns the value in retRGB
 *
 * @param mtlFileName Name of the mtl file in which to look for the material
 * @param mtlName Name of the material to search for
 * @param retRGB Returns the read diffuse value into this array
 */
static void getMaterialFromMtl
    (
    const char* mtlFileName, 
    const char* mtlName, 
    float retRGB[3]
    ) 
{

char actualFileName[1024] = "../../../../emulator/resources/";

strcat(actualFileName, mtlFileName);

FILE* mtlFile = fopen(actualFileName, "rb");

if ( mtlFile == NULL )
{
    /* TODO: Maybe exit gracefully instead of exploding? */
    printf("NOOOO!\n");
    exit(1);
}

char readMtlName[512];

int c;
while ( (c = fgetc(mtlFile)) != EOF )
    {
    switch ( c ) 
        {
        case ' ':
        case '\t':
        case '\n':
            break;
        case '#':
            skipToNextLine(mtlFile);
            break;
        case 'n':
            ungetc(c, mtlFile);
            fscanf(mtlFile, "%511s", readMtlName);

            if ( strcmp(readMtlName, "newmtl") != 0 )
            {
            break;
            }
            fscanf(mtlFile, "%511s", readMtlName);
            if ( strcmp(readMtlName, mtlName) != 0 )
            {
            break;
            }
            getMtlDiffuse(mtlFile, retRGB);
            fclose(mtlFile);
            return ;
            break;
        default:
            break;
        }

    }

fclose(mtlFile);
} /* getMaterialFromMtl */

/**
 * Reads all vertex position entries and pushes them onto the vertexPositionData DARRAY
 *
 * @param file The file we're reading from
 * @param vertexPositionData A pointer to the DARRAY which to push the results onto
 */
static void parseVertex
    (
    FILE* file, 
    float** vertexPositionData
    )
{

int c;
// Hopefully this covers all platforms? although I think I'm the odd one since windows lol
while ( (c = fgetc(file)) != '\n' && c != '\r' ) 
    {
    switch ( c ) 
        {
        case ' ':
        case '\t':
            // ignore
            break;
        default:
            /* Anything else implies there is going to be a float next, go back 1 char and read the float */
            ungetc(c, file);
            float readFloat;
            fscanf(file, "%f", &readFloat);
            (void)DARRAY_PUSH_ESF(vertexPositionData, readFloat);
            break;
        }
    }

} /* parseVertex */

/**
 * Reads the normal index from the end of a single face entry group
 *
 * @param file The file we're reading from
 * @param meshObject The mesh object to add the normal data to
 * @param currentVertexNormalsData A DARRAY containing the current list of defined vertex normals
 */
static void parseFaceNormalIndex
    (
    FILE* file, 
    struct meshObject* meshObject, 
    const float* currentVertexNormalsData
    )
{
    // theres gonna be an index here, just read it bro
    int readInt;
    fscanf(file, "%d", &readInt);
    /* Decrement because obj indices are 1-based */
    readInt--;
    (void)DARRAY_PUSH(meshObject->vertexData, *(currentVertexNormalsData + readInt * 3));
    (void)DARRAY_PUSH(meshObject->vertexData, *(currentVertexNormalsData + readInt * 3 + 1));
    (void)DARRAY_PUSH(meshObject->vertexData, *(currentVertexNormalsData + readInt * 3 + 2));

} /* parseFaceNormalIndex */

/**
 * Reads the texture index from one face group (does nothing with it rn)
 *
 * @param file The file we're reading from
 * @param meshObject The meshObject to push the texture data to
 * @param currentVertexNormalsData DARRAY of previously defined vertex normals data to pass to next parsing stage
 */
static void parseFaceTextureIndex
    (
    FILE* file, 
    struct meshObject* meshObject, 
    const float* currentVertexNormalsData
    ) 
{

/* Notice that there is no loop here */
int c = fgetc(file);
switch ( c ) 
    {
    case '/':
        /* Indicates texture index is omitted, skip to reading the normal index */
        parseFaceNormalIndex(file, meshObject, currentVertexNormalsData);
        break;
    default:
        ungetc(c, file);
        // read the float (does nothing rn)
        int readInt;
        fscanf(file, "%d", &readInt);
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
} /* parseFaceTextureIndex */ 

/**
 * Parses the first component of a single face data group
 *
 * @param file The file we're reading from
 * @param meshObject The mesObject to write data to
 * @param currentVertexPositionData DARRAY of all previously defined vertex coordinates
 * @param currentVertexNormalsData DARRAY of all previously defined vertex normals (just passed on to next stage)
 */
static void parseFaceVertexIndex
    (
    FILE* file, 
    struct meshObject* meshObject, 
    const float* currentVertexPositionData, 
    const float* currentVertexNormalsData
    ) 
{

int readInt;
fscanf(file, "%d", &readInt);
readInt--;
(void)DARRAY_PUSH(meshObject->vertexData, *(currentVertexPositionData + readInt * 3));
(void)DARRAY_PUSH(meshObject->vertexData, *(currentVertexPositionData + readInt * 3 + 1));
(void)DARRAY_PUSH(meshObject->vertexData, *(currentVertexPositionData + readInt * 3 + 2));
// Continue to next state if a / is found
int c = fgetc(file);
if ( c == '/' ) 
    {
    parseFaceTextureIndex(file, meshObject, currentVertexNormalsData);
    }
else
    {
    ungetc(c, file);
    }
} /* parseFaceVertexIndex */

/**
 * Reads all data groups defining a face
 *
 * @param file The file we're reading from
 * @param meshObject The meshObject to write face data to
 * @param currentVertexPositionData DARRAY of all previously read vertex coordinates
 * @param currentVertexNormalsData DARRAY of all previously read vertex normals
 * @param RGB The diffuse value of the currently in-use material
 *
 * @todo The current method of loading color data after every vertex is woefully inefficient, consider other methods 
 */
static void parseFace
    (
    FILE* file, 
    struct meshObject* meshObject, 
    const float* currentVertexPositionData, 
    const float* currentVertexNormalsData, 
    const float RGB[3]
    ) 
{

int c;
while ( (c = fgetc(file)) != '\n' && c != '\r' ) 
    {
    switch ( c ) 
        {
        case ' ':
        case '\t':
            break;
        case '/':
            // found the 
            break;
            
        default:
            // Anything else implies a float next, go back 1 char and read
            ungetc(c, file);
            parseFaceVertexIndex(file, meshObject, currentVertexPositionData, currentVertexNormalsData);
            (void)DARRAY_PUSH(meshObject->vertexData, RGB[0]);
            (void)DARRAY_PUSH(meshObject->vertexData, RGB[1]);
            (void)DARRAY_PUSH(meshObject->vertexData, RGB[2]);
                break;
        }
    }
} /* parseFace */

/**
 * Reads vertex normal data the same way vertex coordintae data is read
 *
 * @param file The file we're reading
 * @param vertexNormalData Pointer to a DARRAY which normal data will be pushed to
 *
 * @todo This function and the parseVertex function could probably be combined into one.
 */
static void parseVertexNormal(FILE* file, float** vertexNormalData)
{
int c;
while ( (c = fgetc(file)) != '\n' && c != '\r' ) 
    {
    switch ( c ) 
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
            (void)DARRAY_PUSH_ESF(vertexNormalData, readFloat);
                break;
        }
    }

} /* parseVertexNormal */



/*******************************************************************************
* END OF FILE                                                                  *
*******************************************************************************/
