/*******************************************************************************
*
* FILE: 
* 		emulator_gui.c
*
* DESCRIPTION: 
*       Entry point for the gui portion of the FC emulator.        	
*       
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
 Includes                                                         
------------------------------------------------------------------------------*/
#include <math.h>
#include <stdlib.h>
#include <stdio.h>

/* necessary define for glad header-only mode */
#include "glad/gl.h" // glad must go before glfw
#include "glfw3.h"
#include "emulator.h"
#include "shaders/shaders.h"
#include "loadAssets/loadAssets.h"
#include "loadAssets/loadAssets.h"
#include "math/lin.h"
#include "containers/darr.h"

#define MAKE_SHADER_PATH(X) "../../../../emulator/src/shaders/"X
#define MAKE_RESOURCES_PATH(X) "../../../../emulator/resources/"X

/*------------------------------------------------------------------------------
 Constants                                                       
------------------------------------------------------------------------------*/
const float vertices[] = {
    -0.5, -0.5, 0,
    0.5, -0.5, 0,
    0, 0.5, 0,
};
/*------------------------------------------------------------------------------
 Globals                                                       
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
 Static Variables
------------------------------------------------------------------------------*/

GLFWwindow* guiWindow = NULL;

/*------------------------------------------------------------------------------
 Static Prototypes                                                       
------------------------------------------------------------------------------*/
static void error_callback
    (
    int error, 
    const char* description
    );

static void framebuffer_size_callback 
    (
    GLFWwindow* window,
    int width,
    int height
    );

static double lastMouseX = 0;
static double lastMouseY = 0;
static void evaluateCursorDiff()
{
glfwGetCursorPos(guiWindow, &lastMouseX, &lastMouseY);
}

static const double sensitivity = 0.02;
static mat4 getUserRotation()
{
    const double epsilon = 0.001;

    const double lmx = lastMouseX;
    const double lmy = lastMouseY;

    evaluateCursorDiff();

    if ( fabs(lastMouseX - lmx) < epsilon && fabs(lastMouseY - lmy) < epsilon)
    {
        return mat4Identity();
    }

    int state = glfwGetMouseButton(guiWindow, GLFW_MOUSE_BUTTON_LEFT);
    if (state != GLFW_PRESS)
    {
        return mat4Identity();
    }

    vec3 dir = vec3Normalize(vec3New(lastMouseX-lmx, -lastMouseY+lmy, 0));
    vec3 rotAxis = vec3Cross(vec3New(0, 0, 1), dir);
    return mat4AxisAngle
            (rotAxis, 
             sensitivity
            );
}



/*------------------------------------------------------------------------------
 Procedures                                                     
------------------------------------------------------------------------------*/
static struct fileVertexData objData;
void emulator_gui_init
    (
    void
    )
{
if (!glfwInit()) {
    fprintf(stderr, "[GUI]: GLFW Initialization failed");
    exit(1);
}

glfwSetErrorCallback(error_callback);

glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
guiWindow = glfwCreateWindow(640, 480, "SDR HW Emulator", NULL, NULL);
glfwHideWindow(guiWindow);

if (!guiWindow) 
    {

    fprintf(stderr, "[GUI]: Window Initialization failed");
    glfwTerminate();
    exit(1);

    }

glfwSetFramebufferSizeCallback(guiWindow, framebuffer_size_callback);

glfwMakeContextCurrent(guiWindow);

/* Load OpenGL functions */
gladLoadGL(glfwGetProcAddress);

/* Load obj */
objData = loadVertexDataFromOBJ(MAKE_RESOURCES_PATH("FC_REV2.obj"));
/*
for (size_t i = 0; i < objData.vertexDataCount; i+=3) {
    printf("VERTEX: %.2f, %.2f, %.2f\n", objData.vertexData[i], objData.vertexData[i+1], objData.vertexData[i+2]);
}
for (size_t i = 0; i < objData.faceIndexDataCount; i++) {
    printf("FACE INDEX: %u\n", objData.faceIndexData[i]);
}
*/
/*
for (size_t i = 0; i < DARRAY_SIZE(objData.vertexNormalsData); i++)
{
printf("VERTEX NORMAL: %f\n", DARRAY_GET(objData.vertexNormalsData, i));
}
*/
/*
for (size_t i = 0; i < DARRAY_SIZE(objData.vertexNormalsIndices); i++)
{
printf("VERTEX NORMAL INDEX: %d; CORRESPONDING NORMAL: (%f, %f, %f)\n", 
                DARRAY_GET(objData.vertexNormalsIndices, i), 
                *(objData.vertexNormalsData + objData.vertexNormalsIndices[i]*3),
                *(objData.vertexNormalsData + objData.vertexNormalsIndices[i]*3+1),
                *(objData.vertexNormalsData + objData.vertexNormalsIndices[i]*3+2));
}
*/
int runningtotal = 0;
for (size_t i = 0; i < DARRAY_SIZE(objData.fileMaterialsData); i++)
{
    struct fileMaterials mat = objData.fileMaterialsData[i];
    printf("VERTS: %d, COLOR: (%f, %f, %f)\n", mat.numIndiciesUsingMat, mat.r, mat.g, mat.b);
    runningtotal += mat.numIndiciesUsingMat;
}
printf("TOTAL INDICES USING MATS: %d\n", runningtotal);
}

void emulator_gui_teardown
    (
    void
    )
{
    
glfwDestroyWindow(guiWindow);
glfwTerminate();

}


/*******************************************************************************
*                                                                              *
* PROCEDURE:                                                                   * 
* 		emulator_gui_main                                                      *
*                                                                              *
* DESCRIPTION:                                                                 * 
*       Emulator GUI entry point.                                              *
*                                                                              *
*******************************************************************************/
void emulator_gui_main
    (
    void
    ) 
{

GLuint VAO;
glGenVertexArrays(1, &VAO);
glBindVertexArray(VAO);

//TODO: THE METHOD FOR LOADING VERTICES:
// The load obj function will populate a new array of type struct (containing material names and no. of indices defined with that material)
// We can then load from that and interleave the data here
float *realVBOData= DARRAY_NEW(float, 100);
printf("FACE INDEX SIZE: %d\n", DARRAY_SIZE(objData.faceIndexData));
printf("FACE NORMALS INDEX SIZE: %d\n", DARRAY_SIZE(objData.vertexNormalsIndices));
for (size_t i = 0; i < DARRAY_SIZE(objData.faceIndexData); i++)
{
DARRAY_PUSH(realVBOData, *(objData.vertexData + objData.faceIndexData[i]*3));
DARRAY_PUSH(realVBOData, *(objData.vertexData + objData.faceIndexData[i]*3+1));
DARRAY_PUSH(realVBOData, *(objData.vertexData + objData.faceIndexData[i]*3+2));

DARRAY_PUSH(realVBOData, *(objData.vertexNormalsData + objData.vertexNormalsIndices[i]*3));
DARRAY_PUSH(realVBOData, *(objData.vertexNormalsData + objData.vertexNormalsIndices[i]*3+1));
DARRAY_PUSH(realVBOData, *(objData.vertexNormalsData + objData.vertexNormalsIndices[i]*3+2));
}

GLuint VBO;
glGenBuffers(1, &VBO);
glBindBuffer(GL_ARRAY_BUFFER, VBO);
//glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
glBufferData(GL_ARRAY_BUFFER, sizeof(float) * DARRAY_SIZE(realVBOData), realVBOData, GL_STATIC_DRAW);
//DARRAY_FREE(objData.vertexData);
glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
glEnableVertexAttribArray(0);
glEnableVertexAttribArray(1);

// TODO: I'm gonna need to write something to reorder my normals so they arent borked
/*
for (size_t i = 0; i < DARRAY_SIZE(realVBOData); i+=6)
{
printf("VERTEX COORDS: (%f, %f, %f), VERTEX NORMALS (%f, %f, %f)\n", realVBOData[i], realVBOData[i+1], realVBOData[i+2], realVBOData[i+3], realVBOData[i+4], realVBOData[i+5]);
}
*/
//TODO: I think im gna have to give up on using EBO for now and just convert indices to raw vertices so I can use normals more easily.
// It seems you can only have one VBO per VAO, so I will just interleave normal and vert data

GLuint shaderProgram = 
    genShaderProgramFromSources
        (
        MAKE_SHADER_PATH("default.vert"),
        MAKE_SHADER_PATH("default.frag")
        );

glfwShowWindow(guiWindow);
glfwRequestWindowAttention(guiWindow);
glfwFocusWindow(guiWindow);

vec3 axis = vec3Normalize(vec3New(1, 1, 1));
mat4 model = mat4Identity();
mat4_debugprint(model);
vec3 camPos = vec3New(0, 0, 85); 
vec3 camTarget = vec3New(0, 0, 0);
vec3 camUp = vec3New(0, 1, 0);
mat4 view = mat4LookAt(camPos, camTarget, camUp);
mat4 proj = mat4Proj(1.57, 16.0/9.0, 1, 300);

int modelUniformLocation = glGetUniformLocation(shaderProgram, "model");
int viewUniformLocation = glGetUniformLocation(shaderProgram, "view");
int projUniformLocation = glGetUniformLocation(shaderProgram, "proj");

//mat4_debugprint(proj);

//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
glEnable(GL_DEPTH_TEST);

printf("[GUI STARTUP SUCCESSFUL]: Rise and shine\n");
while (!glfwWindowShouldClose(guiWindow)) 
    {
    model = mat4Mult(getUserRotation(), model);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(shaderProgram);
    glUniformMatrix4fv(modelUniformLocation, 1, GL_FALSE, model.data);
    glUniformMatrix4fv(viewUniformLocation, 1, GL_FALSE, view.data);
    glUniformMatrix4fv(projUniformLocation, 1, GL_FALSE, proj.data);
    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, DARRAY_SIZE(realVBOData));

    glfwSwapBuffers(guiWindow);
    glfwPollEvents();
    }

} /* emulator_gui_main */

/*******************************************************************************
*                                                                              *
* PROCEDURE:                                                                   * 
* 		error_callback                                                         *
*                                                                              *
* DESCRIPTION:                                                                 * 
*       Invoked by GLFW to print errors to console.                            *
*                                                                              *
*******************************************************************************/
static void error_callback
    (
    int error, 
    const char* description
    ) 
{

fprintf(stderr, "[GLFW ERROR]: %s\n", description);

}

/*******************************************************************************
*                                                                              *
* PROCEDURE:                                                                   * 
* 		framebuffer_size_callback                                              *
*                                                                              *
* DESCRIPTION:                                                                 * 
*       Invoked by GLFW to resize the rendering area within the window         *
*                                                                              *
*******************************************************************************/
static void framebuffer_size_callback 
    (
    GLFWwindow* window,
    int width,
    int height
    )
{

glViewport(0, 0, width, height);

}
