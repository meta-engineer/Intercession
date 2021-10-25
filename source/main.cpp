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

#include "cube_mesh.h"
#include "cube_2_mesh.h"

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

    if (glfwGetKey(window, GLFW_KEY_GRAVE_ACCENT) == GLFW_PRESS)
        cm.set_use_perspective(!cm.get_use_perspective());
}

int loadGLTexture(const char* path)
{
    unsigned int texture_ID;
    glGenTextures(1, &texture_ID);
    glBindTexture(GL_TEXTURE_2D, texture_ID);

    // wrapping options?
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    // color for non-mapped surface (using GL_CLAMP_TO_BORDER)
    //float borderColor[] = { 0.1f, 0.1f, 0.1f, 1.0f };
    //glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

    // texture mapping options
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // texturing data
    int texWidth, texHeight, texChannels;
    stbi_set_flip_vertically_on_load(true);
    unsigned char *texData = stbi_load(path, &texWidth, &texHeight, &texChannels, 0);
    if (texData)
    {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, texWidth, texHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, texData);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else
    {
        std::cerr << "Failed to load texture: " << path << std::endl;
    }
    stbi_image_free(texData);

    return texture_ID;
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

    cm.set_target(glm::vec3(0.0f, 0.0f, 0.0f));
    cm.move_vertical(1.0f);

    // on window resize callback
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    // process_input is just a plain function called in render loop
    // on mouse move callback
    glfwSetCursorPosCallback(window, mouse_callback);
    // set mouse capture mode
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    // on mouse scroll callback
    glfwSetScrollCallback(window, scroll_callback);

    // ******************** Vertex Data Objects *************************
    // store all VAO info in objects
    Cube2Mesh default_cubes[10];
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
    for (int i = 0; i < 10; i++)
    {
        default_cubes[i].set_origin(positions[i]);
    }

    CubeMesh light_source;
    glm::vec3 staticColor(0.8f, 0.9f, 0.6f);
    light_source.set_origin(glm::vec3(0.0f, 2.0f, 0.8f));
    light_source.rotate(glm::radians(55.0f), glm::vec3(1.0, 0.0, 1.0));
    light_source.set_uniform_scale(0.1f);

    // ******************** Shading Objects *************************
    // use ShaderManager to build shader program from filenames
    ShaderManager sm(
        "source/shaders/projection_lighting.vs", 
        "source/shaders/better_lighting.fs"
    );
    sm.activate();
    //sm.setVec3("light.direction", glm::vec3(-0.2f, -1.0f, -0.3f)); 
    sm.setVec3("pLight.position", light_source.get_origin());
    sm.setVec3("pLight.attenuation", glm::vec3(1.0f, 0.14f, 0.07f));
    sm.setVec3("pLight.ambient",  staticColor * 0.1f);
    sm.setVec3("pLight.diffuse",  staticColor * 1.0f);
    sm.setVec3("pLight.specular", staticColor * 1.0f);

    glm::vec3 spotColor(0.6f, 0.8f, 1.0f);
    sm.setVec3("sLight.position", cm.get_position());
    sm.setVec3("sLight.direction", cm.get_direction());
    sm.setVec3("sLight.attenuation", glm::vec3(1.0f, 0.14f, 0.07f));
    sm.setFloat("sLight.innerCos", 0.99f);
    sm.setFloat("sLight.outerCos", 0.92f);
    sm.setVec3("sLight.ambient",  spotColor * 0.1f);
    sm.setVec3("sLight.diffuse",  spotColor * 1.0f);
    sm.setVec3("sLight.specular", spotColor * 1.0f);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // texture object to be bound later
    unsigned int box_texture_ID = loadGLTexture("resources/container2.png");
    unsigned int box_specular_ID= loadGLTexture("resources/container2_specular.png");

    ShaderManager light_sm(
        "source/shaders/projection_lighting.vs",
        "source/shaders/light_source.fs"
    );
    light_sm.activate();
    light_sm.setVec3("lightColor", staticColor);


    // ******************** Render Loop *************************
    float lastTimeVal = 0;
    float FPS = 100;
    float frameTimeDelta = 1.0/FPS;
    
    while(!glfwWindowShouldClose(window))
    {
        float timeVal = glfwGetTime();
        float deltaTime = timeVal - lastTimeVal;
        if (deltaTime < frameTimeDelta) continue;
        std::cout << "FPS: " << 1.0/(timeVal - lastTimeVal) << "\r" << std::flush;
        process_input(window, deltaTime);
        lastTimeVal = timeVal;

        glClearColor(0.1f, 0.2f, 0.3f, 1.0f);
        glEnable(GL_DEPTH_TEST);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // bind to texture "unit" 0
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, box_texture_ID);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, box_specular_ID);

        sm.activate();
        sm.setInt("material.diffuse", 0); // texture "unit" id not object id
        sm.setInt("material.specular", 1);
        sm.setFloat("material.shininess", 32.0f);
        sm.setVec3("sLight.position", cm.get_position());
        sm.setVec3("sLight.direction", cm.get_direction());
        sm.setVec3("viewPos", cm.get_position());
        sm.setMat4("world_to_view", cm.get_lookAt());
        sm.setMat4("projection", cm.get_projection());
        for (unsigned int i = 0; i < 10; i++)
        {
            default_cubes[i].rotate(glm::radians(i / 10.0f), glm::vec3(1.0f, 0.7f, 0.4f));

            sm.setMat4("model_to_world", default_cubes[i].get_model_transform());
            default_cubes[i].invoke_draw();
        }

        light_sm.activate();
        light_sm.setMat4("model_to_world", light_source.get_model_transform());
        light_sm.setMat4("world_to_view", cm.get_lookAt());
        light_sm.setMat4("projection", cm.get_projection());
        light_source.invoke_draw();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    std::cout << "Stopped rendering, terminating..." << std::endl;
    glfwTerminate();
    return 0;
}
