// Copyright Pleep inc. 2021

// std libraries
#include <iostream>
#include <fstream>
#include <string>

// external
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#define STB_IMAGE_IMPLEMENTATION // must define before include?
#include <stb_image.h>

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
        // coordinates          // color            // texture coordinates
         0.5f,  0.45f,  0.0f,   1.0f, 0.0f, 0.0f,   1.5f, 1.5f,
        -0.5f,  0.45f,  0.0f,   1.0f, 1.0f, 0.0f,   -0.5f, 1.5f,
        -0.5f, -0.45f,  0.0f,   0.0f, 1.0f, 1.0f,   -0.5f, -0.5f,
         0.5f, -0.45f,  0.0f,   0.0f, 0.0f, 1.0f,   1.5f, -0.5f
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
        "source/shaders/texture_basic.vs", 
        "source/shaders/mix_textures.fs"
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
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3* sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6* sizeof(float)));
    glEnableVertexAttribArray(2);

    // texture object
    unsigned int texture_ID;
    glGenTextures(1, &texture_ID);
    glBindTexture(GL_TEXTURE_2D, texture_ID);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    // color for non-mapped surface (using GL_CLAMP_TO_BORDER)
    float borderColor[] = { 0.1f, 0.1f, 0.1f, 1.0f };
    //glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
    // texture mapping options
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // texturing data
    int texWidth, texHeight, texChannels;
    stbi_set_flip_vertically_on_load(true);
    unsigned char *texData = stbi_load("resources/awesomeface.png", &texWidth, &texHeight, &texChannels, 0);
    if (texData)
    {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, texWidth, texHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, texData);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else
    {
        std::cerr << "Failed to load texture" << std::endl;
    }
    stbi_image_free(texData);

    //texture object 2
    unsigned int texture_2_ID;
    glGenTextures(1, &texture_2_ID);
    glBindTexture(GL_TEXTURE_2D, texture_2_ID);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    // texture mapping options
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // texturing data
    int tex2Width, tex2Height, tex2Channels;
    unsigned char *tex2Data = stbi_load("resources/wall.jpg", &tex2Width, &tex2Height, &tex2Channels, 0);
    if (texData)
    {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, tex2Width, tex2Height, 0, GL_RGB, GL_UNSIGNED_BYTE, tex2Data);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else
    {
        std::cerr << "Failed to load texture" << std::endl;
    }
    stbi_image_free(tex2Data);

    // clear binds
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glUseProgram(0);


    // render loop
    sm.activate();
    sm.setInt("texture1", 0);
    sm.setInt("texture2", 1);

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

        //update shader uniform
        float alphaVal = sin(timeVal) / 2.0f + 0.5f;
        sm.setFloat("globalAlpha", alphaVal);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture_ID);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, texture_2_ID);

        glBindVertexArray(VAO_ID);
        //glDrawArrays(GL_TRIANGLES, 0, 3); // from attribute 0 to (0+3)
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    std::cout << "Stopped rendering, terminating..." << std::endl;
    // cleanup
    glDeleteVertexArrays(1, &VAO_ID);
    glDeleteBuffers(1, &VBO_ID);
    glDeleteBuffers(1, &EBO_ID);
    glfwTerminate();
    return 0;
}
