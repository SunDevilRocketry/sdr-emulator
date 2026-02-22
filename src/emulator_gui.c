/* necessary define for glad header-only mode */
#define GLAD_GL_IMPLEMENTATION
#include "glad/gl.h"
#include "glfw3.h"
#include "emulator.h"
#include "shaders/readShader.h"
#include <stdlib.h>
#include <stdio.h>


float vertices[] = {
    -0.5, -0.5, 0,
    0.5, -0.5, 0,
    0, 0.5, 0,
};

static void error_callback
    (
     int error, 
     const char* description
     ) 
{
fprintf(stderr, "[GLFW ERROR]: %s\n", description);
}

static void framebuffer_size_callback 
(
GLFWwindow* window,
int width,
int height
)
{

glViewport(0, 0, width, height);

}

void emulator_gui_main
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
    GLFWwindow* guiWindow = glfwCreateWindow(640, 480, "SDR HW Emulator", NULL, NULL);
    if (!guiWindow) {
        fprintf(stderr, "[GUI]: Window Initialization failed");
        glfwTerminate();
        exit(1);
    }

    glfwSetFramebufferSizeCallback(guiWindow, framebuffer_size_callback);

    glfwMakeContextCurrent(guiWindow);

    /* Load OpenGL functions */
    gladLoadGL(glfwGetProcAddress);

    GLuint VAO;
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    GLuint VBO;
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    int success;
    char infoLog[512];

    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    const char* vertexShaderSource = readShaderSource("../../../../emulator/src/shaders/default.vert");
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);
    free(vertexShaderSource);


    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        fprintf(stderr, "vShader compilation failed:\n \t%s\n", infoLog);
    }

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    const char* fragmentShaderSource = readShaderSource("../../../../emulator/src/shaders/default.frag");
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    free(fragmentShaderSource);

    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        fprintf(stderr, "fShader compilation failed:\n \t%s\n", infoLog);
    }

    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    glUseProgram(shaderProgram);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        fprintf(stderr, "Shader linkage failed:\n \t%s\n", infoLog);
        exit(1);
    }

    while (!glfwWindowShouldClose(guiWindow)) 
    {
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(shaderProgram);
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, 3);

        glfwSwapBuffers(guiWindow);
        glfwPollEvents();
    }
    

    glfwDestroyWindow(guiWindow);
    glfwTerminate();
}
