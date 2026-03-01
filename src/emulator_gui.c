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
#include "shaders/readShader.h"
#include "emulator_loadAssets.h"
#include "math/lin.h"

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

GLuint VBO;
glGenBuffers(1, &VBO);
glBindBuffer(GL_ARRAY_BUFFER, VBO);
//glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
glBufferData(GL_ARRAY_BUFFER, sizeof(float) * objData.vertexDataCount, objData.vertexData, GL_STATIC_DRAW);
free(objData.vertexData); // temp

GLuint EBO;
glGenBuffers(1, &EBO);
glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(int) * objData.faceIndexDataCount, objData.faceIndexData, GL_STATIC_DRAW);
glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
free(objData.faceIndexData);

glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
glEnableVertexAttribArray(0);

GLuint shaderProgram = 
    genShaderProgramFromSources
        (
        MAKE_SHADER_PATH("default.vert"),
        MAKE_SHADER_PATH("default.frag")
        );

glfwShowWindow(guiWindow);
glfwRequestWindowAttention(guiWindow);
glfwFocusWindow(guiWindow);

mat4 model = mat4RotY(0);
vec3 camPos = vec3New(0, 20, 100); 
vec3 camTarget = vec3New(0, -40, 0);
vec3 camUp = vec3New(0, 1, 0);
mat4 view = mat4LookAt(camPos, camTarget, camUp);
mat4 proj = mat4Proj(1.57, 16.0/9.0, 1, 500);
mat4_debugprint(view);

int modelUniformLocation = glGetUniformLocation(shaderProgram, "model");
int viewUniformLocation = glGetUniformLocation(shaderProgram, "view");
int projUniformLocation = glGetUniformLocation(shaderProgram, "proj");

mat4_debugprint(proj);

glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

printf("[GUI STARTUP SUCCESSFUL]: Rise and shine\n");
while (!glfwWindowShouldClose(guiWindow)) 
    {
    model = mat4RotY(glfwGetTime());
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(shaderProgram);
    glBindVertexArray(VAO);
    glUniformMatrix4fv(modelUniformLocation, 1, GL_FALSE, model.data);
    glUniformMatrix4fv(viewUniformLocation, 1, GL_FALSE, view.data);
    glUniformMatrix4fv(projUniformLocation, 1, GL_FALSE, proj.data);
    //glDrawArrays(GL_TRIANGLES, 0, objData.vertexDataCount);
    glDrawElements(GL_TRIANGLES, objData.faceIndexDataCount* 6, GL_UNSIGNED_INT, 0);

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
