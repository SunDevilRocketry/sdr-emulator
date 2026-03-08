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
#include <string.h>

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
struct directionalLight {
    vec3 direction;

    vec3 ambient;
    vec3 diffuse;
    // no specular
};

/*------------------------------------------------------------------------------
 Static Variables
------------------------------------------------------------------------------*/

GLFWwindow* guiWindow = NULL;
static float powerLEDColor[3] = {0, 1, 0};
static float statusLEDColor[3] = {0, 0, 0};

/*------------------------------------------------------------------------------
 Static Prototypes                                                       
------------------------------------------------------------------------------*/
static void glfwKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);

static void GLAPIENTRY openGLErrorCallback
    (
    GLenum source,
    GLenum type,
    GLenum id,
    GLenum severity,
    GLsizei length,
    const GLchar* message,
    const void* userParam
    );

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
void setGUIStatusLED(const float r, const float g, const float b)
{
statusLEDColor[0] = r;
statusLEDColor[1] = g;
statusLEDColor[2] = b;
}

static struct meshObject* objData;
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
glfwSetKeyCallback(guiWindow, glfwKeyCallback);

glfwMakeContextCurrent(guiWindow);

/* Load OpenGL functions */
gladLoadGL(glfwGetProcAddress);
glEnable(GL_DEBUG_OUTPUT);
glDebugMessageCallback(openGLErrorCallback, 0);

/* Load obj */
objData = loadVertexDataFromOBJ(MAKE_RESOURCES_PATH("FC_REV2.obj"));
printf("READ COMPLETE\n");
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

GLuint defaultVAO;
glGenVertexArrays(1, &defaultVAO);
glBindVertexArray(defaultVAO);

float* defaultVBOData = DARRAY_NEW(float, 100);
for (size_t i = 0; i < DARRAY_SIZE(objData); i++)
{
    if ( strcmp("Power_Light", objData[i].objName) == 0 || strcmp("Status_Light", objData[i].objName) == 0) 
    {
        continue;
    }
    for(size_t vs = 0; vs < DARRAY_SIZE(objData[i].vertexData); vs++)
    {

        DARRAY_PUSH(defaultVBOData, objData[i].vertexData[vs]);
    }
}

GLuint defaultVBO;
glGenBuffers(1, &defaultVBO);
glBindBuffer(GL_ARRAY_BUFFER, defaultVBO);
glBufferData(GL_ARRAY_BUFFER, sizeof(float) * DARRAY_SIZE(defaultVBOData), defaultVBOData, GL_STATIC_DRAW);
glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)0);
glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(3 * sizeof(float)));
glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(6 * sizeof(float)));
glEnableVertexAttribArray(0);
glEnableVertexAttribArray(1);
glEnableVertexAttribArray(2);

GLuint defaultShaderProgram = 
    genShaderProgramFromSources
        (
        MAKE_SHADER_PATH("default.vert"),
        MAKE_SHADER_PATH("default.frag")
        );


float* powerLightVBOData = DARRAY_NEW(float, 100);
for (size_t i = 0; i < DARRAY_SIZE(objData); i++)
{
    if ( strcmp("Power_Light", objData[i].objName) != 0 ) 
    {
        continue;
    }
    for(size_t vs = 0; vs < DARRAY_SIZE(objData[i].vertexData); vs++)
    {

        DARRAY_PUSH(powerLightVBOData, objData[i].vertexData[vs]);
    }
}
GLuint powerLightVAO;
glGenVertexArrays(1, &powerLightVAO);
glBindVertexArray(powerLightVAO);

GLuint powerLightVBO;
glGenBuffers(1, &powerLightVBO);
glBindBuffer(GL_ARRAY_BUFFER, powerLightVBO);
glBufferData(GL_ARRAY_BUFFER, sizeof(float) * DARRAY_SIZE(powerLightVBOData), powerLightVBOData, GL_STATIC_DRAW);
glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)0);
glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(3 * sizeof(float)));
glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(6 * sizeof(float)));
glEnableVertexAttribArray(0);
glEnableVertexAttribArray(1);
glEnableVertexAttribArray(2);

float* statusLightVBOData = DARRAY_NEW(float, 100);
for (size_t i = 0; i < DARRAY_SIZE(objData); i++)
{
    if ( strcmp("Status_Light", objData[i].objName) != 0 ) 
    {
        continue;
    }
    for(size_t vs = 0; vs < DARRAY_SIZE(objData[i].vertexData); vs++)
    {
        DARRAY_PUSH(statusLightVBOData, objData[i].vertexData[vs]);
    }
}

GLuint statusLightVAO;
glGenVertexArrays(1, &statusLightVAO);
glBindVertexArray(statusLightVAO);

GLuint statusLightVBO;
glGenBuffers(1, &statusLightVBO);
glBindBuffer(GL_ARRAY_BUFFER, statusLightVBO);
glBufferData(GL_ARRAY_BUFFER, sizeof(float) * DARRAY_SIZE(statusLightVBOData), statusLightVBOData, GL_STATIC_DRAW);
glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)0);
glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(3 * sizeof(float)));
glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(6 * sizeof(float)));
glEnableVertexAttribArray(0);
glEnableVertexAttribArray(1);
glEnableVertexAttribArray(2);

GLuint LEDShaderProgram = 
    genShaderProgramFromSources
        (
        MAKE_SHADER_PATH("LED.vert"),
        MAKE_SHADER_PATH("LED.frag")
        );


mat4 model = mat4Identity();
vec3 camPos = vec3New(0, 0, 85); 
vec3 camTarget = vec3New(0, 0, 0);
vec3 camUp = vec3New(0, 1, 0);
mat4 view = mat4LookAt(camPos, camTarget, camUp);
mat4 proj = mat4Proj(1.57, 16.0/9.0, 1, 300);

int modelUniformLocation = glGetUniformLocation(defaultShaderProgram, "model");
int viewUniformLocation = glGetUniformLocation(defaultShaderProgram, "view");
int projUniformLocation = glGetUniformLocation(defaultShaderProgram, "proj");
int directionalLightDirUniformLocation = glGetUniformLocation(defaultShaderProgram, "globalLight.direction");
int directionalLightAmbientUniformLocation = glGetUniformLocation(defaultShaderProgram, "globalLight.ambient");
int directionalLightDiffuseUniformLocation = glGetUniformLocation(defaultShaderProgram, "globalLight.diffuse");

int LEDModelUniformLocation = glGetUniformLocation(LEDShaderProgram, "model");
int LEDViewlUniformLocation = glGetUniformLocation(LEDShaderProgram, "view");
int LEDProjUniformLocation = glGetUniformLocation(LEDShaderProgram, "proj");
int LEDEmissionColorUniformLocation = glGetUniformLocation(LEDShaderProgram, "emissionColor");

struct directionalLight globalLight = {
    .direction= vec3New(-0.5, -0.5, -1),
    .ambient = vec3New(0.1, 0.1, 0.1),
    .diffuse = vec3New(0.9, 0.9, 0.9),
};


glEnable(GL_DEPTH_TEST);

glfwShowWindow(guiWindow);
glfwRequestWindowAttention(guiWindow);
glfwFocusWindow(guiWindow);

int defaultDrawCount = DARRAY_SIZE(defaultVBOData)/9;
int powerLightDrawCount = DARRAY_SIZE(powerLightVBOData)/9;
int statusLightDrawCount = DARRAY_SIZE(statusLightVBOData)/9;

for (size_t i = 0; i < DARRAY_SIZE(objData); i++)
{
DARRAY_FREE(objData[i].vertexData);
}
DARRAY_FREE(objData);
DARRAY_FREE(defaultVBOData);
DARRAY_FREE(powerLightVBOData);
DARRAY_FREE(statusLightVBOData);

printf("[GUI STARTUP SUCCESSFUL]: Rise and shine\n");
while (!glfwWindowShouldClose(guiWindow)) 
    {
    model = mat4Mult(getUserRotation(), model);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glBindVertexArray(defaultVAO);
    glUseProgram(defaultShaderProgram);
    glUniformMatrix4fv(modelUniformLocation, 1, GL_FALSE, model.data);
    glUniformMatrix4fv(viewUniformLocation, 1, GL_FALSE, view.data);
    glUniformMatrix4fv(projUniformLocation, 1, GL_FALSE, proj.data);

    glUniform3fv(directionalLightDirUniformLocation, 1, globalLight.direction.data);
    glUniform3fv(directionalLightAmbientUniformLocation, 1, globalLight.ambient.data);
    glUniform3fv(directionalLightDiffuseUniformLocation, 1, globalLight.diffuse.data);

    glDrawArrays(GL_TRIANGLES, 0, defaultDrawCount);

    glUseProgram(LEDShaderProgram);
    glUniformMatrix4fv(LEDModelUniformLocation, 1, GL_FALSE, model.data);
    glUniformMatrix4fv(LEDViewlUniformLocation, 1, GL_FALSE, view.data);
    glUniformMatrix4fv(LEDProjUniformLocation, 1, GL_FALSE, proj.data);

    glBindVertexArray(statusLightVAO);
    glUniform3fv(LEDEmissionColorUniformLocation, 1, statusLEDColor);
    glDrawArrays(GL_TRIANGLES, 0, statusLightDrawCount);

    glBindVertexArray(powerLightVAO);
    glUniform3fv(LEDEmissionColorUniformLocation, 1, powerLEDColor);
    glDrawArrays(GL_TRIANGLES, 0, powerLightDrawCount);

    glfwSwapBuffers(guiWindow);
    glfwPollEvents();
    }
    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR) {
        printf("ERROR: %d", err);
    }
} /* emulator_gui_main */

static void glfwKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_R && action == GLFW_PRESS && mods & GLFW_MOD_CONTROL) 
    {
    printf("IGNITE SET\n");
    setIgniteFlag(true);
    }
}

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

static void GLAPIENTRY openGLErrorCallback
    (
    GLenum source,
    GLenum type,
    GLenum id,
    GLenum severity,
    GLsizei length,
    const GLchar* message,
    const void* userParam
    )
{
if (type == GL_DEBUG_TYPE_ERROR) 
    {
    fprintf(stderr, "GLERROR: %s\n", message);
    }
}
