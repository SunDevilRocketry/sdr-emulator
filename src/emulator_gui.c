/**
 * @file emulator_gui.c
 *
 * Contains GUI specific routines for the FC emulator.
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
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/*------------------------------------------------------------------------------
 Project Includes                                                         
------------------------------------------------------------------------------*/
#include "glad/gl.h" // glad must go before glfw
#include "glfw3.h"
#include "emulator.h"
#include "shaders/shaders.h"
#include "loadAssets/loadAssets.h"
#include "loadAssets/loadAssets.h"
#include "math/lin.h"
#include "containers/darr.h"

/*------------------------------------------------------------------------------
 Macros
------------------------------------------------------------------------------*/
/**
 * @def MAKE_SHADER_PATH(X)
 * Concatenates the given string literal with the path to the emulator shader directory
 * @param X string literal to concatenate
 * @warning Don't test the limits of this macro, it will probably break
 */
#define MAKE_SHADER_PATH(X) "../../emulator/src/shaders/"X
/**
 * @def MAKE_RESOURCES_PATH(X)
 * Concatenates the given string literal with the path to the emulator resource directory
 * @param X string literal to concatenate
 * @warning Don't test the limits of this macro, it will probably break
 */
#define MAKE_RESOURCES_PATH(X) "../../emulator/resources/"X

/*------------------------------------------------------------------------------
 Structs 
------------------------------------------------------------------------------*/

/**
 *
 * Defines parameters for a directional type light to be passed to the fragment shader
 *
 */
struct directionalLight {
    vec3 direction; /**< direction The direction light is emitted */

    vec3 ambient;   /**< ambient The ambient light strength of the light */
    vec3 diffuse;   /**< diffuse The diffuse strength of the light */
};

/*------------------------------------------------------------------------------
 Static Variables
------------------------------------------------------------------------------*/

static GLFWwindow* guiWindow = NULL;
static float powerLEDColor[3] = {0, 1, 0};
static float statusLEDColor[3] = {0, 0, 0};

static double lastMouseX = 0;
static double lastMouseY = 0;
static const double sensitivity = 0.02;

static struct meshObject* objData;

/*------------------------------------------------------------------------------
 Static Prototypes                                                       
------------------------------------------------------------------------------*/
static void glfwKeyCallback
    (
    GLFWwindow* window, 
    int key, 
    int scancode, 
    int action, 
    int mods
    );

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

static void glfw_error_callback
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

static mat4 getUserRotation
    (
    void
    );

/*------------------------------------------------------------------------------
 Procedures                                                     
------------------------------------------------------------------------------*/

/**
 * Sets the static variable @ref statusLEDColor to contain the passed RGB value
 *
 * @param r The red component of the light
 * @param g The green component of the light
 * @param b The blue component of the light
 *
 * @note The permitted range for each RGB value is [0, 1]; The expected range is {0, 1}
 */
void set_gui_status_led 
    (
    const float r, 
    const float g, 
    const float b
    )
{

statusLEDColor[0] = r;
statusLEDColor[1] = g;
statusLEDColor[2] = b;

} /* setGUIStatusLED */

/**
 * The entry point for the GUI
 *
 * @warning Make sure to call @ref emulator_gui_init before this function.
 */
void emulator_gui_main
    (
    void
    ) 
{

if ( !glfwInit() ) 
    {
    fprintf(stderr, "[GUI]: GLFW Initialization failed");
    exit(1);
    }

/* Set window-independant callbacks */
glfwSetErrorCallback(glfw_error_callback);

/* Set initial window parameters and create */
glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
guiWindow = glfwCreateWindow(640, 480, "SDR HW Emulator", NULL, NULL);

if ( !guiWindow ) 
    {
    fprintf(stderr, "[GUI]: Window Initialization failed");
    glfwTerminate();
    exit(1);
    }

/* Set window dependant callbacks */
glfwSetFramebufferSizeCallback(guiWindow, framebuffer_size_callback);
glfwSetKeyCallback(guiWindow, glfwKeyCallback);
glfwMakeContextCurrent(guiWindow);

/* Load OpenGL functions */
gladLoadGL(glfwGetProcAddress);
glEnable(GL_DEBUG_OUTPUT);
glEnable(GL_DEPTH_TEST);
glDebugMessageCallback(openGLErrorCallback, 0);

/* Load FC obj */
objData = loadVertexDataFromOBJ(MAKE_RESOURCES_PATH("FC_REV2.obj"));
printf("FC OBJ READ COMPLETE\n");

/* Create the VAO for non-LED geometry */
GLuint defaultVAO;
glGenVertexArrays(1, &defaultVAO);
glBindVertexArray(defaultVAO);

/* Collect non-LED vertex data into one buffer */
float* defaultVBOData = DARRAY_NEW(float, 100);
for (size_t i = 0; i < DARRAY_SIZE(objData); i++)
{
    if ( strcmp("Power_Light", objData[i].objName) == 0 || strcmp("Status_Light", objData[i].objName) == 0) 
    {
        continue;
    }
    for(size_t vs = 0; vs < DARRAY_SIZE(objData[i].vertexData); vs++)
    {

        (void)DARRAY_PUSH(defaultVBOData, objData[i].vertexData[vs]);
    }
}

/* Create VBO and buffer non-LED vertex data */
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

/* Load non-LED shaders */
GLuint defaultShaderProgram = 
    genShaderProgramFromSources
        (
        MAKE_SHADER_PATH("default.vert"),
        MAKE_SHADER_PATH("default.frag")
        );


/* Create VAO for power light */
GLuint powerLightVAO;
glGenVertexArrays(1, &powerLightVAO);
glBindVertexArray(powerLightVAO);

/* Collect power light vertex data into one buffer */
float* powerLightVBOData = DARRAY_NEW(float, 100);
for (size_t i = 0; i < DARRAY_SIZE(objData); i++)
{
    if ( strcmp("Power_Light", objData[i].objName) != 0 ) 
    {
        continue;
    }
    for(size_t vs = 0; vs < DARRAY_SIZE(objData[i].vertexData); vs++)
    {

        (void)DARRAY_PUSH(powerLightVBOData, objData[i].vertexData[vs]);
    }
}

/* Buffer power light data into VBO */
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


/* Create status light VAO */
GLuint statusLightVAO;
glGenVertexArrays(1, &statusLightVAO);
glBindVertexArray(statusLightVAO);

/* Collect status LED vertex data into one buffer */
float* statusLightVBOData = DARRAY_NEW(float, 100);
for (size_t i = 0; i < DARRAY_SIZE(objData); i++)
{
    if ( strcmp("Status_Light", objData[i].objName) != 0 ) 
    {
        continue;
    }
    for(size_t vs = 0; vs < DARRAY_SIZE(objData[i].vertexData); vs++)
    {
        (void)DARRAY_PUSH(statusLightVBOData, objData[i].vertexData[vs]);
    }
}

/* Buffer LED vertex data into VBO */
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

/* Generate LED shaders */
GLuint LEDShaderProgram = 
    genShaderProgramFromSources
        (
        MAKE_SHADER_PATH("LED.vert"),
        MAKE_SHADER_PATH("LED.frag")
        );


/* Initialize positioning matrices */
mat4 model = mat4Identity();
vec3 camPos = vec3New(0, 0, 85); 
vec3 camTarget = vec3New(0, 0, 0);
vec3 camUp = vec3New(0, 1, 0);
mat4 view = mat4LookAt(camPos, camTarget, camUp);
mat4 proj = mat4Proj(1.57, 16.0/9.0, 1, 300);

/* Get uniform buffer location IDs from LED and non-LED shaders */
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

/* Define the global directional light attributes */
struct directionalLight globalLight = {
    .direction= vec3New(-0.5, -0.5, -1),
    .ambient = vec3New(0.1, 0.1, 0.1),
    .diffuse = vec3New(0.9, 0.9, 0.9),
};




/* Calculate number of vertices to draw so we can free the vertex data */
int defaultDrawCount = DARRAY_SIZE(defaultVBOData)/9;
int powerLightDrawCount = DARRAY_SIZE(powerLightVBOData)/9;
int statusLightDrawCount = DARRAY_SIZE(statusLightVBOData)/9;

/* Free all vertex data */
for (size_t i = 0; i < DARRAY_SIZE(objData); i++)
{
DARRAY_FREE(objData[i].vertexData);
}
DARRAY_FREE(objData);
DARRAY_FREE(defaultVBOData);
DARRAY_FREE(powerLightVBOData);
DARRAY_FREE(statusLightVBOData);

/* Show the window to the user */
glfwShowWindow(guiWindow);
glfwRequestWindowAttention(guiWindow);
glfwFocusWindow(guiWindow);

printf("[GUI STARTUP SUCCESSFUL]: Rise and shine\n");

printf("[Emulator]: Press CTRL + R to arm.\n");
/* GUI main loop */
while (!glfwWindowShouldClose(guiWindow)) 
    {
    model = mat4Mult(getUserRotation(), model);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    /* Draw non-LED geometry */
    glBindVertexArray(defaultVAO);
    glUseProgram(defaultShaderProgram);
    glUniformMatrix4fv(modelUniformLocation, 1, GL_FALSE, model.data);
    glUniformMatrix4fv(viewUniformLocation, 1, GL_FALSE, view.data);
    glUniformMatrix4fv(projUniformLocation, 1, GL_FALSE, proj.data);

    glUniform3fv(directionalLightDirUniformLocation, 1, globalLight.direction.data);
    glUniform3fv(directionalLightAmbientUniformLocation, 1, globalLight.ambient.data);
    glUniform3fv(directionalLightDiffuseUniformLocation, 1, globalLight.diffuse.data);

    glDrawArrays(GL_TRIANGLES, 0, defaultDrawCount);

    /* Draw status light */
    glUseProgram(LEDShaderProgram);
    glUniformMatrix4fv(LEDModelUniformLocation, 1, GL_FALSE, model.data);
    glUniformMatrix4fv(LEDViewlUniformLocation, 1, GL_FALSE, view.data);
    glUniformMatrix4fv(LEDProjUniformLocation, 1, GL_FALSE, proj.data);

    glBindVertexArray(statusLightVAO);
    glUniform3fv(LEDEmissionColorUniformLocation, 1, statusLEDColor);
    glDrawArrays(GL_TRIANGLES, 0, statusLightDrawCount);

    /* Draw power light */
    glBindVertexArray(powerLightVAO);
    glUniform3fv(LEDEmissionColorUniformLocation, 1, powerLEDColor);
    glDrawArrays(GL_TRIANGLES, 0, powerLightDrawCount);

    glfwSwapBuffers(guiWindow);
    glfwPollEvents();
    }
} /* emulator_gui_main */

/**
 * Frees internal GUI resources; intended to be called before program termination
 *
 * @warning You may not call any functions that require gui state to be intialized 
 * @note I'm not really happy with this function. It assumes the GUI state to be in the main loop or after which is not necessarily true in the case of a SIGINT. 
 * Can't really clean up before that state unless I increase the amount of static variables though.
 */
void emulator_gui_teardown
    (
    void
    )
{
    
glfwDestroyWindow(guiWindow);
glfwTerminate();

} /* emulator_gui_teardown */

/*------------------------------------------------------------------------------
 Static Procedures                                                     
------------------------------------------------------------------------------*/

/**
 * @brief Called by GLFW when a keyboard input is recieved. 
 * Will trigger the ignite flag when CTRL + R is pressed
 *
 * @param window The window which recieved the input
 * @param key The key pressed
 * @param scancode Platform specific key details
 * @param action The state of the key (e.g. Is key pressed?)
 * @param mods Additional key information (e.g. is CTRL pressed?)
 *
 * @note See GLFW documentation for more information
 */
static void glfwKeyCallback
    (
    GLFWwindow* window, 
    int key, 
    int scancode, 
    int action, 
    int mods
    )
{

    if ( key == GLFW_KEY_R && action == GLFW_PRESS && mods & GLFW_MOD_CONTROL ) 
    {
    printf("IGNITE SET\n");
    set_ignite_flag(true);
    }

} /* glfwKeyCallback */

/**
 * @brief Called by GLFW when an internal error occurs
 * Will print the encountered error to stderr
 *
 * @param error Code of error
 * @param description Description of the error which occured
 *
 * @note See GLFW documentation for more information
 */
static void glfw_error_callback
    (
    int error, 
    const char* description
    ) 
{

fprintf(stderr, "[GLFW ERROR]: %s\n", description);

} /* error_callback */

/**
 * @brief Called by GLFW when the window framebuffer is resized
 * Resizes the openGL viewport to match the new framebuffer size
 *
 * @param window The window which was resized
 * @param width New framebuffer width
 * @param height New framebuffer height
 *
 * @note See GLFW documentation for more information
 */
static void framebuffer_size_callback 
    (
    GLFWwindow* window,
    int width,
    int height
    )
{

glViewport(0, 0, width, height);

} /* framebuffer_size_callback */

/**
 * @brief Called by OpenGL when an error occurs
 * Prints error messages to stderr
 *
 * @param source The source which produced the error message
 * @param type The type of message
 * @param severity Identifies the importance of the message
 * @param id An ID of poorly documented usefulness
 * @param message Null-terminated string containing the error message
 *
 * @note See OpenGL documentation for more details
 */
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
if ( type == GL_DEBUG_TYPE_ERROR ) 
    {
    fprintf(stderr, "GLERROR: %s\n", message);
    }

} /* openGLErrorCallback */

/**
 * @brief Calculates a rotation matrix based on mouse delta
 *
 * Returns a rotation matrix about the axis obtained when crossing the user's mouse delta with the +Z axis
 *
 * @return @ref Roration matrix describing the mouse rotation of the FC
 *
 * @todo Make rotation speed proportional to the size of the user's mouse delta
 */
static mat4 getUserRotation
    (
    void
    )
{
    const double epsilon = 0.001;

    const double lmx = lastMouseX;
    const double lmy = lastMouseY;

    glfwGetCursorPos(guiWindow, &lastMouseX, &lastMouseY);

    if ( fabs(lastMouseX - lmx) < epsilon && fabs(lastMouseY - lmy) < epsilon )
    {
        return mat4Identity();
    }

    int state = glfwGetMouseButton(guiWindow, GLFW_MOUSE_BUTTON_LEFT);
    if ( state != GLFW_PRESS )
    {
        return mat4Identity();
    }

    vec3 dir = vec3Normalize( vec3New(lastMouseX-lmx, -lastMouseY+lmy, 0) );
    vec3 rotAxis = vec3Cross(vec3New(0, 0, 1), dir);
    return mat4AxisAngle
            (rotAxis, 
             sensitivity
            );

} /* getUserRotation */
