/* necessary define for glad header-only mode */
#define GLAD_GL_IMPLEMENTATION
#include "glad/gl.h"
#include "glfw3.h"
#include "emulator.h"
#include <stdlib.h>
#include <stdio.h>


void emulator_gui_main
    (
    void
    ) 
{
    if (!glfwInit()) {
        fprintf(stderr, "[GUI]: GLFW Initialization failed");
        exit(1);
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow* guiWindow = glfwCreateWindow(640, 480, "SDR HW Emulator", NULL, NULL);
    if (!guiWindow) {
        fprintf(stderr, "[GUI]: Window Initialization failed");
        glfwTerminate();
        exit(1);
    }

    glfwMakeContextCurrent(guiWindow);

    gladLoadGL(glfwGetProcAddress);

    while (!glfwWindowShouldClose(guiWindow)) 
    {
        glClear(GL_COLOR_BUFFER_BIT);
        glfwSwapBuffers(guiWindow);
        glfwPollEvents();
    }
    

    glfwDestroyWindow(guiWindow);
    glfwTerminate();


}
