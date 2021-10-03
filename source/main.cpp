// Copyright Pleep inc. 2021

// std libraries
#include <iostream>
#include <fstream>
#include <string>

// external
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

// internal
#include "build_config.h"
#ifdef USE_VIEW_MANAGER
#include "view_manager/view_manager.h"
#endif

#include "test.h"
#include "shader_manifest.h"

// TODO: move these
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

void process_input(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    {
        glfwSetWindowShouldClose(window, true);
    }
}

std::string load_shader_string(std::string filename)
{
    std::string ret_str;
    std::ifstream fs(filename, std::ios::in);
    if(!fs.is_open()) {
        std::cerr << "Could not read file " << filename << ". File does not exist." << std::endl;
        return "";
    }
    std::string line = "";
    while(!fs.eof()) {
        std::getline(fs, line);
        ret_str.append(line + "\n");
    }
    fs.close();
    return ret_str;
}


int main(int argc, char** argv) {
    std::cout << argv[0] << " Version " << BUILD_VERSION_MAJOR << "." << BUILD_VERSION_MINOR << std::endl;
#if defined(_DEBUG)
    std::cout << "Using Debug build" << std::endl;
#elif defined(NDEBUG)
    std::cout << "Using Release build" << std::endl;
#endif

    std::cout << glfwGetVersionString() << std::endl << std::endl;
    
#ifdef USE_VIEW_MANAGER
    std::cout << view_manager::get_context_id() << std::endl;
    std::cout << view_manager::get_glfw_version_major() << std::endl;
#endif

    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(800, 600, "OpenGL_2021", NULL, NULL);
    if (!window)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    // GLAD manages function pointers for OpenGL, so it must be init before using OpenGL functions
    // inform GLAD of the function to load the address of the OS-specific OpenGL function pointers
    //  (glfw knows them and supplies this function)
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to init GLAD" << std::endl;
        return -1;
    }

    // GLFW "window" equal to the gl "viewport"
    glViewport(0, 0, 800, 600);

    // on window resize callback
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // rendering data
    float vertices[] = {
         0.5f,  0.45f,  0.0f, 1.0f, 0.0f, 0.0f,
        -0.5f,  0.45f,  0.0f, 1.0f, 1.0f, 0.0f,
        -0.5f, -0.45f,  0.0f, 0.0f, 1.0f, 1.0f,
         0.5f, -0.45f,  0.0f, 0.0f, 0.0f, 1.0f
    };
    unsigned int indicies[] = {
        0,3,1,
        3,2,1
    };

    // GL "Objects"
    // store attribute calls and VBO reference
    unsigned int VAO_ID;
    glGenVertexArrays(1, &VAO_ID);
    glBindVertexArray(VAO_ID);

    // store our verticies
    unsigned int VBO_ID;
    glGenBuffers(1, &VBO_ID);

    // feed in vertex data
    glBindBuffer(GL_ARRAY_BUFFER, VBO_ID);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // store the vertex indexing
    unsigned int EBO_ID;
    glGenBuffers(1, &EBO_ID);

    // feed in index data
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO_ID);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indicies), indicies, GL_STATIC_DRAW);

    // use ShaderManifest to build shader program from filenames
    ShaderManifest sm(
        "source/shaders/color_basic.vs", 
        "source/shaders/color_basic.fs"
    );

    // Set the interpretation of our VBO structure for the vertex shader input
    //  (glVertexAttribPointer takes data from the currently bound vbo)
    //  Arguments:
    //  Attrbute to configure (referenced in shader)
    //  size of attribute
    //  type of contained data
    //  normalize data?
    //  stride (space between consecutive attributes)
    //  offset of data in the buffer
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3* sizeof(float)));
    glEnableVertexAttribArray(1);

    // clear binds
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glUseProgram(0);


    // render loop
    float lastTimeVal = 0;
    float FPS = 60;
    float frameTimeDelta = 1.0/FPS;
    while(!glfwWindowShouldClose(window))
    {
        process_input(window);

        float timeVal = glfwGetTime();
        if (timeVal - lastTimeVal < frameTimeDelta) continue;
        std::cout << "FPS: " << 1.0/(timeVal - lastTimeVal) << "\r" << std::flush;
        lastTimeVal = timeVal;

        glClearColor(0.2f, 0.4f, 0.5f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        sm.activate();

        //update shader uniform
        float alphaVal = sin(timeVal) / 2.0f + 0.5f;
        sm.setFloat("globalAlpha", alphaVal);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glBindVertexArray(VAO_ID);
        //glDrawArrays(GL_TRIANGLES, 0, 3); // from attribute 0 to (0+3)
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        glBindVertexArray(VAO_ID);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    std::cout << "Stopped rendering, terminating..." << std::endl;
    // cleanup
    glfwTerminate();
    return 0;
}
