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

    std::cout << glfwGetVersionString() << std::endl;
    
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
         0.5f,  0.45f,  0.0f,
        -0.5f,  0.45f,  0.0f,
        -0.5f, -0.45f,  0.0f,
         0.5f, -0.45f,  0.0f
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

    // vertex shader
    unsigned int VS_ID;
    VS_ID = glCreateShader(GL_VERTEX_SHADER);

    // load shader from file to  char array
    std::string VS_str = load_shader_string("source/shaders/vertex_basic.vert");
    const char *VS_src = VS_str.c_str();

    // shader object, num source code strings, source code string, NULL?
    glShaderSource(VS_ID, 1, &VS_src, NULL);
    glCompileShader(VS_ID);

    // check compile errors
    int success;
    char infoLog[512];
    glGetShaderiv(VS_ID, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(VS_ID, 512, NULL, infoLog);
        std::cout << "Vertex shader compilation failed\n" << infoLog << std::endl;
    }

    // fragment shader
    unsigned int FS_ID;
    FS_ID = glCreateShader(GL_FRAGMENT_SHADER);
    std::string FS_str = load_shader_string("source/shaders/fragment_basic.frag");
    const char *FS_src = FS_str.c_str();
    glShaderSource(FS_ID, 1, &FS_src, NULL);
    glCompileShader(FS_ID);

    // check compile errors
    glGetShaderiv(FS_ID, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(VS_ID, 512, NULL, infoLog);
        std::cout << "Fragment shader compilation failed\n" << infoLog << std::endl;
    }

    // The Shader Program
    unsigned int SP_ID;
    SP_ID = glCreateProgram();
    glAttachShader(SP_ID, VS_ID);
    glAttachShader(SP_ID, FS_ID);
    glLinkProgram(SP_ID);

    //check for linking errors
    glGetProgramiv(SP_ID, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(SP_ID, 512, NULL, infoLog);
        std::cout << "Shader Program linking failed\n" << infoLog << std::endl;
    }

    // activate shader program
    glUseProgram(SP_ID);

    // delete compiled shader references
    glDeleteShader(VS_ID);
    glDeleteShader(FS_ID);

    // Set the interpretation of our VBO structure for the vertex shader input
    //  (glVertexAttribPointer takes data from the currently bound vbo)
    //  Arguments:
    //  Attrbute to configure (referenced in shader)
    //  size of attribute
    //  type of contained data
    //  normalize data?
    //  stride (space between consecutive attributes)
    //  offset of data in the buffer
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // clear binds
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glUseProgram(0);

    // Vertex Array object. Stores vertex attribute calls

    // render loop
    while(!glfwWindowShouldClose(window))
    {
        process_input(window);

        glClearColor(0.2f, 0.4f, 0.5f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

        glUseProgram(SP_ID);
        glBindVertexArray(VAO_ID);
        //glDrawArrays(GL_TRIANGLES, 0, 3); // from attribute 0 to (0+3)
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        glBindVertexArray(VAO_ID);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // cleanup
    glfwTerminate();
    return 0;
}
