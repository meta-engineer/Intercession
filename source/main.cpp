// Copyright Pleep inc. 2021

// std libraries
#include <iostream>
#include <fstream>
#include <string>

// external
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define STB_IMAGE_IMPLEMENTATION // must define before include?
#include <stb_image.h>

// internal
#include "build_config.h"
#ifdef USE_VIEW_MANAGER
#include "view_manager.h"
#endif

#include "test.h"
#include "shader_manager.h"
#include "camera_manager.h"

CameraManager cm(glm::vec3(0.0f, 0.0f, 4.0f));

// TODO: move these
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    cm.set_view_height(height);
    cm.set_view_width(width);
    // should viewport be controlled by camera manager...? TBD
    glViewport(0, 0, width, height);
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    static float lastX = -1;
    static float lastY = -1;
    const float sensitivity = 0.05f;

    float xoffset = lastX == -1 ? 0 : xpos - lastX;
    float yoffset = lastY == -1 ? 0 : lastY - ypos;

    lastX = xpos;
    lastY = ypos;

    cm.turn_yaw(xoffset * sensitivity);
    cm.turn_pitch(yoffset * sensitivity);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    cm.set_view_fov(cm.get_view_fov() - (float)yoffset);
}

void process_input(GLFWwindow *window, float deltaTime)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    {
        glfwSetWindowShouldClose(window, true);
    }

    const float spd = 2.5f * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        cm.move_foreward(spd);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        cm.move_foreward(-spd);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        cm.move_horizontal(-spd);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        cm.move_horizontal(spd);
    if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS)
        cm.move_global_vertical(-spd);
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
        cm.move_global_vertical(spd);
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
    std::cout << "Using view_manager library" << std:: endl;
    std::cout << view_manager::get_context_id() << std::endl;
    std::cout << view_manager::get_glfw_version_major() << std::endl;
#endif

    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(cm.get_view_width(), cm.get_view_height(), "OpenGL_2021", NULL, NULL);
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
    glViewport(0, 0, cm.get_view_width(), cm.get_view_height());

    // on window resize callback
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    // process_input is just a plain function called in render loop
    // on mouse move callback
    glfwSetCursorPosCallback(window, mouse_callback);
    // set mouse capture mode
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    // on mouse scroll callback
    glfwSetScrollCallback(window, scroll_callback);

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
    glm::vec3 positions[] = {
        glm::vec3( 0.0f,  0.0f,  0.0f), 
        glm::vec3( 2.0f,  5.0f, -15.0f), 
        glm::vec3(-1.5f, -2.2f, -2.5f),  
        glm::vec3(-3.8f, -2.0f, -12.3f),  
        glm::vec3( 2.4f, -0.4f, -3.5f),  
        glm::vec3(-1.7f,  3.0f, -7.5f),  
        glm::vec3( 1.3f, -2.0f, -2.5f),  
        glm::vec3( 1.5f,  2.0f, -2.5f), 
        glm::vec3( 1.5f,  0.2f, -1.5f), 
        glm::vec3(-1.3f,  1.0f, -1.5f)  
    };

    cm.set_target(glm::vec3(0.0f, 0.0f, 0.0f));
    glm::mat4 model_to_world = glm::mat4(1.0f);
    model_to_world = glm::rotate(model_to_world, glm::radians(-55.0f), glm::vec3(1.0f, 0.0f, 0.0f));

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

    // use ShaderManager to build shader program from filenames
    ShaderManager sm(
        "source/shaders/projection_basic.vs", 
        "source/shaders/mix_textures.fs"
    );
    // shader uniforms
    sm.activate();
    sm.setInt("texture1", 0);
    sm.setInt("texture2", 1);

    float lastTimeVal = 0;
    float FPS = 100;
    float frameTimeDelta = 1.0/FPS;
    // render loop
    while(!glfwWindowShouldClose(window))
    {
        float timeVal = glfwGetTime();
        float deltaTime = timeVal - lastTimeVal;
        if (deltaTime < frameTimeDelta) continue;

        std::cout << "FPS: " << 1.0/(timeVal - lastTimeVal) << "\r" << std::flush;
        process_input(window, deltaTime);
        lastTimeVal = timeVal;

        glClearColor(0.2f, 0.4f, 0.5f, 1.0f);
        glEnable(GL_DEPTH_TEST);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        //update shader uniforms
        float alphaVal = sin(timeVal) / 2.0f + 0.5f;
        sm.setFloat("globalAlpha", alphaVal);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture_ID);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, texture_2_ID);

        glBindVertexArray(VAO_ID);
        for (unsigned int i = 0; i < 10; i++)
        {
            glm::mat4 model_to_world = glm::mat4(1.0f);
            model_to_world = glm::translate(model_to_world, positions[i]);
            model_to_world = glm::rotate(model_to_world, (float)glfwGetTime() * glm::radians(i * 10.0f), glm::vec3(1.0f, 0.7f, 0.4f));
            sm.setMat4("model_to_world", model_to_world);

            sm.setMat4("world_to_view", cm.get_lookAt());
            sm.setMat4("projection", cm.get_projection());
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        }
        //glDrawArrays(GL_TRIANGLES, 0, 3); // from attribute 0 to (0+3)

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
