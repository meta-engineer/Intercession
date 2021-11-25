// Copyright Pleep inc. 2021

// std libraries
#include <iostream>
#include <fstream>
#include <string>
#include <algorithm>

// external
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <assimp/scene.h>

//#define STB_IMAGE_IMPLEMENTATION // must define before include?
#include <stb_image.h>

// internal
#include "build_config.h"
#ifdef USE_VIEW_MANAGER
#include "view_manager.h"
#endif

#include "test.h"
#include "shader_manager.h"
#include "camera_manager.h"

#include "entity.h"
#include "collective_entity.h"
#include "model.h"
#include "cube_model.h"
#include "quad_model.h"

#include "vertex_group.h"
#include "screen_plane_vertex_group.h"
#include "skybox_vertex_group.h"
#include "cube_vertex_group.h"

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

unsigned int loadGLCubeMapTexture(std::vector<std::string> filenames)
{
    unsigned int texture_ID;
    glGenTextures(1, &texture_ID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, texture_ID);

    // options for cube_maps
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    // data
    int texWidth, texHeight, texChannels;
    for (unsigned int i = 0; i < std::min(filenames.size(), (size_t)6); i++)
    {
        unsigned char *texData = stbi_load(filenames[i].c_str(), &texWidth, &texHeight, &texChannels, 0);
        if (texData)
        {
            GLenum format;
            if (texChannels == 3)
                format = GL_RGB;
            else if (texChannels == 4)
                format = GL_RGBA;
            else // if (texChannels == 1)
                format = GL_RED;

            glTexImage2D(
                GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 
                0, format, texWidth, texHeight, 0, format, GL_UNSIGNED_BYTE, texData
            );
        }
        else
        {
            std::cerr << "MODEL::loadGLTexture Failed to load texture: " << filenames[i] << std::endl;
        }
        stbi_image_free(texData);
    }

    // clear
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
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
    // MSAA buffer done by glfw
    //glfwWindowHint(GLFW_SAMPLES, 4);

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

    // MSAA enable
    //glEnable(GL_MULTISAMPLE); 

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
    std::vector<Entity> default_cubes;
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
        Entity cb(create_cube_model_ptr("resources/container2.png", "resources/container2_specular.png"));
        default_cubes.push_back(std::move(cb));
        default_cubes[i].set_origin(positions[i]);
    }

    Entity light_source(create_cube_model_ptr());
    light_source.set_origin(glm::vec3(0.0f, 2.0f, 0.8f));
    light_source.rotate(glm::radians(55.0f), glm::vec3(1.0, 0.0, 1.0));
    light_source.set_uniform_scale(0.1f);

    // NOTE: models passed as moved or temporary pointers to give entity exclusive control
    Entity frog(std::make_unique<Model>("resources/normal_frog.obj"));

    // Example for resetting model afterward
    //std::unique_ptr<Model> frog_model = std::make_unique<Model>("resources/12268_banjofrog_v1_L3.obj");
    //frog.reset_graphics_model(std::move(frog_model));
    // OR
    //frog.reset_graphics_model(create_cube_model_ptr("resources/12268_banjofrog_diffuse.jpg"));

    frog.set_origin(glm::vec3(0, 0.5, 0));
    frog.set_uniform_scale(0.2f);

    Entity grass(create_quad_model_ptr("resources/grass.png"));
    grass.set_origin(glm::vec3(0.0f, 1.5f, -1.0f));

    //CollectiveEntity grasses = CollectiveEntity(create_quad_model_ptr("resources/grass.png"));
    CollectiveEntity grasses = CollectiveEntity(create_cube_model_ptr("resources/blending_transparent_window.png"));
    grasses.set_origin(glm::vec3(0.0f, 3.0f, -1.0f));
    grasses.rotate(glm::radians(90.0f), glm::vec3(0,0,1));

    // Fun
    unsigned int num_blades = 10000;
    std::vector<glm::mat4> grass_transforms;
    srand(glfwGetTime());
    float radius = 10.0;
    float offset = 3.0f;
    for (unsigned int i = 0; i < num_blades; i++)
    {
        glm::mat4 transform = glm::mat4(1.0f);
        float angle = (float)i / (float)num_blades * 360.0f;
        float displacement = (rand() % (int)(2 * offset * 100)) / 100.0f - offset;
        float x = sin(angle) * radius + displacement;
        displacement = (rand() % (int)(2 * offset * 100)) / 100.0f - offset;
        float y = displacement * 0.4f; // keep height of field smaller compared to width of x and z
        displacement = (rand() % (int)(2 * offset * 100)) / 100.0f - offset;
        float z = cos(angle) * radius + displacement;
        transform = glm::translate(transform, glm::vec3(x, y, z));

        // 2. scale: scale between 0.05 and 0.25f
        float scale = (rand() % 20) / 100.0f + 0.05;
        transform = glm::scale(transform, glm::vec3(scale));

        // 3. rotation: add random rotation around a (semi)randomly picked rotation axis vector
        float rotAngle = (rand() % 360);
        transform = glm::rotate(transform, rotAngle, glm::vec3(0.4f, 0.6f, 0.8f));

        grass_transforms.push_back(transform);
    }
    grasses.set_instance_transforms(grass_transforms);

    Entity floor(create_quad_model_ptr("resources/wall.jpg", "resources/wall.jpg"));
    floor.set_origin(glm::vec3(0.0f, -1.5f, 0.0));
    floor.rotate(glm::radians(-70.0f), glm::vec3(1.0, 0.0, 0.0));
    floor.set_uniform_scale(10.0f);


    // ******************** Shading Objects *************************
    
    glm::vec3 staticColor(1.0f, 1.0f, 1.0f);
    glm::vec3 spotColor(0.6f, 0.8f, 1.0f);

    // use ShaderManager to build shader program from filenames
    ShaderManager sm(
        "source/shaders/projection_lighting.vs",
        "source/shaders/advanced_lighting.fs"
    );
    sm.activate();
    sm.setInt("num_ray_lights", 1);
    sm.setVec3("rLights[0].direction", glm::vec3(-0.2f, -1.0f, -0.3f)); 
    sm.setVec3("rLights[0].attenuation", glm::vec3(1.0f, 0.14f, 0.07f));
    sm.setVec3("rLights[0].ambient",  staticColor * 0.1f);
    sm.setVec3("rLights[0].diffuse",  staticColor * 1.0f);
    sm.setVec3("rLights[0].specular", staticColor * 1.0f);

    sm.setInt("num_point_lights", 1);
    sm.setVec3("pLights[0].position", light_source.get_origin());
    sm.setVec3("pLights[0].attenuation", glm::vec3(1.0f, 0.14f, 0.07f));
    sm.setVec3("pLights[0].ambient",  staticColor * 0.1f);
    sm.setVec3("pLights[0].diffuse",  staticColor * 1.0f);
    sm.setVec3("pLights[0].specular", staticColor * 1.0f);

    //sm.setInt("num_spot_lights", 1);
    sm.setVec3("sLights[0].position", cm.get_position());
    sm.setVec3("sLights[0].direction", cm.get_direction());
    sm.setVec3("sLights[0].attenuation", glm::vec3(1.0f, 0.14f, 0.07f));
    sm.setFloat("sLights[0].innerCos", 0.99f);
    sm.setFloat("sLights[0].outerCos", 0.92f);
    sm.setVec3("sLights[0].ambient",  spotColor * 0.1f);
    sm.setVec3("sLights[0].diffuse",  spotColor * 1.0f);
    sm.setVec3("sLights[0].specular", spotColor * 1.0f);

    ShaderManager instances_sm(
        "source/shaders/projection_instances.vs",
        "source/shaders/advanced_lighting.fs"
    );
    
    instances_sm.activate();
    instances_sm.setInt("num_ray_lights", 1);
    instances_sm.setVec3("rLights[0].direction", glm::vec3(-0.2f, -1.0f, -0.3f)); 
    instances_sm.setVec3("rLights[0].attenuation", glm::vec3(1.0f, 0.14f, 0.07f));
    instances_sm.setVec3("rLights[0].ambient",  staticColor * 0.1f);
    instances_sm.setVec3("rLights[0].diffuse",  staticColor * 1.0f);
    instances_sm.setVec3("rLights[0].specular", staticColor * 1.0f);

    instances_sm.setInt("num_point_lights", 1);
    instances_sm.setVec3("pLights[0].position", light_source.get_origin());
    instances_sm.setVec3("pLights[0].attenuation", glm::vec3(1.0f, 0.14f, 0.07f));
    instances_sm.setVec3("pLights[0].ambient",  staticColor * 0.1f);
    instances_sm.setVec3("pLights[0].diffuse",  staticColor * 1.0f);
    instances_sm.setVec3("pLights[0].specular", staticColor * 1.0f);

    ShaderManager normal_visualizer_sm(
        "source/shaders/viewspace_normal.vs",
        "source/shaders/vertex_normals.gs",
        "source/shaders/fixed_color.fs"
    );

    ShaderManager light_sm(
        "source/shaders/projection_lighting.vs",
        "source/shaders/light_source.fs"
    );
    light_sm.activate();
    light_sm.setVec3("lightColor", staticColor);

    // hold view transforms in a uniform buffer (as all mesh's use the same per frame)
    unsigned int UBO_ID;
    glGenBuffers(1, &UBO_ID);
    glBindBuffer(GL_UNIFORM_BUFFER, UBO_ID);
    glBufferData(GL_UNIFORM_BUFFER, 2 * sizeof(glm::mat4), NULL, GL_STATIC_DRAW);
    // set buffer to bind point 0
    glBindBufferRange(GL_UNIFORM_BUFFER, 0, UBO_ID, 0, 2 * sizeof(glm::mat4));

    // bind Shader's global uniform block view_transform to 0
    sm.activate();
    glUniformBlockBinding(sm.SP_ID, glGetUniformBlockIndex(sm.SP_ID, "view_transforms"), 0);

    normal_visualizer_sm.activate();
    glUniformBlockBinding(normal_visualizer_sm.SP_ID, glGetUniformBlockIndex(normal_visualizer_sm.SP_ID, "view_transforms"), 0);

    instances_sm.activate();
    glUniformBlockBinding(instances_sm.SP_ID, glGetUniformBlockIndex(instances_sm.SP_ID, "view_transforms"), 0);

    // keep light_sm without universal uniforms
    //light_sm.activate();
    //glUniformBlockBinding(light_sm.SP_ID, glGetUniformBlockIndex(light_sm.SP_ID, "view_transform"), 0);

    // rendering options
    sm.activate();
    glClearColor(0.1f, 0.2f, 0.3f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    // culling (with correct index orientation)
    //glEnable(GL_CULL_FACE);
    //glCullFace(GL_FRONT);
    // wireframe
    //glPolygonMode( GL_FRONT_AND_BACK, GL_LINE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    // enable stencil operations
    glEnable(GL_STENCIL_TEST);

    // skybox cubemap
    ShaderManager skybox_sm(
        "source/shaders/skybox.vs",
        "source/shaders/skybox.fs"
    );
    // order (as defined by opengl) is right, left, top, bottom, back, front
    std::vector<std::string> skybox_filenames{
        "resources/lake_skybox/right.jpg",
        "resources/lake_skybox/left.jpg",
        "resources/lake_skybox/top.jpg",
        "resources/lake_skybox/bottom.jpg",
        "resources/lake_skybox/front.jpg",
        "resources/lake_skybox/back.jpg"
    };
    unsigned int skybox_texture_ID = loadGLCubeMapTexture(skybox_filenames);
    skybox_sm.activate();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, skybox_texture_ID);
    skybox_sm.setInt("cube_map", 0);
    CubeVertexGroup skybox;

    frog.reset_graphics_model_environment_maps(skybox_texture_ID);
    for (int i = 0; i < default_cubes.size(); i++)
    {
        default_cubes[i].reset_graphics_model_environment_maps(skybox_texture_ID);
    }

    // finally framebuffers! ************************************
    ShaderManager screen_texture_sm(
        "source/shaders/screen_texture.vs",
        "source/shaders/convolutions.fs"
    );
    ScreenPlaneVertexGroup screen_plane;

    unsigned int FBO_ID;
    glGenFramebuffers(1, &FBO_ID);
    glBindFramebuffer(GL_FRAMEBUFFER, FBO_ID);

    /*
    // render to a texture... (This is used for color)
    unsigned int rendered_texture_ID;
    glGenTextures(1, &rendered_texture_ID);
    glBindTexture(GL_TEXTURE_2D, rendered_texture_ID);
    // allocate memory for buffer, NULL data
    // should probably use a unique cm
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, cm.get_view_width(), cm.get_view_height(), 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0); // clear
    // attach buffers (with type information) to the framebuffer object
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, rendered_texture_ID, 0);

    // render to a RenderBuffer (this used for depth/stencil buffer)
    unsigned int RBO_ID;
    glGenRenderbuffers(1, &RBO_ID);
    glBindRenderbuffer(GL_RENDERBUFFER, RBO_ID);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, cm.get_view_width(), cm.get_view_height());
    glBindRenderbuffer(GL_RENDERBUFFER, 0); // clear
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, RBO_ID);
    */

    // use a multisample texture instead
    unsigned int MSAA_render_texture_ID;
    glGenTextures(1, &MSAA_render_texture_ID);
    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, MSAA_render_texture_ID);
    glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 4 /*samples*/, GL_RGB, cm.get_view_width(), cm.get_view_height(), GL_TRUE);
    // GL_TRUE -> use identical sample locations and the same number of subsamples for each texel
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);
    // attach to framebuffer
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, MSAA_render_texture_ID, 0);

    // use a multisampled renderbuffer instead
    unsigned int MSAA_RBO_ID;
    glGenRenderbuffers(1, &MSAA_RBO_ID);
    glBindRenderbuffer(GL_RENDERBUFFER, MSAA_RBO_ID);
    glRenderbufferStorageMultisample(GL_RENDERBUFFER, 4 /*samples*/, GL_DEPTH24_STENCIL8, cm.get_view_width(), cm.get_view_height());
    glBindRenderbuffer(GL_RENDERBUFFER, 0); // clear
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, MSAA_RBO_ID);

    // check frambuffer
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        std::cout << "New Framebuffer was not complete, rebinding to default" << std::endl;
        glBindFramebuffer(GL_FRAMEBUFFER, 0); // back to the screen
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0); // back to the screen


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

        // enable framebuffer for post-processing
        glBindFramebuffer(GL_FRAMEBUFFER, FBO_ID);

        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );//| GL_STENCIL_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);

        // set uniform buffers for all shaders
        glBindBuffer(GL_UNIFORM_BUFFER, UBO_ID);
        glBufferSubData(GL_UNIFORM_BUFFER, 0,                 sizeof(glm::mat4), &(cm.get_lookAt()));
        // could optimize further by only resetting projection if fov changes
        glBufferSubData(GL_UNIFORM_BUFFER, sizeof(glm::mat4), sizeof(glm::mat4), &(cm.get_projection()));

        sm.activate();
        sm.setVec3("sLights[0].position", cm.get_position());
        sm.setVec3("sLights[0].direction", cm.get_direction());
        sm.setVec3("viewPos", cm.get_position());
        //sm.setMat4("world_to_view", cm.get_lookAt());
        //sm.setMat4("projection", cm.get_projection());

        for (unsigned int i = 0; i < default_cubes.size(); i++)
        {
            default_cubes[i].rotate(glm::radians(i / 10.0f), glm::vec3(1.0f, 0.7f, 0.4f));
            default_cubes[i].invoke_draw(sm);
        }
        frog.invoke_draw(sm);
        //frog.invoke_draw(normal_visualizer_sm);

        light_sm.activate();
        light_sm.setMat4("model_to_world", light_source.get_model_transform());
        light_sm.setMat4("world_to_view", cm.get_lookAt());
        light_sm.setMat4("projection", cm.get_projection());

        light_source.invoke_draw(light_sm);

        sm.activate();
        floor.invoke_draw(sm);

        // skybox
        glDepthFunc(GL_LEQUAL); // so skybox only passes if there is nothing else in buffer
        skybox_sm.activate();
        // cut out top-left 3x3 matrix (ommiting translation)
        skybox_sm.setMat4("world_to_view", glm::mat4(glm::mat3(cm.get_lookAt())));
        skybox_sm.setMat4("projection", cm.get_projection());
        //skybox.invoke_draw();
        // reset depth buffer
        glDepthFunc(GL_LESS);
        
        instances_sm.activate();
        instances_sm.setVec3("sLights[0].position", cm.get_position());
        instances_sm.setVec3("sLights[0].direction", cm.get_direction());
        instances_sm.setVec3("viewPos", cm.get_position());
        // draw all available instances
        grasses.invoke_instanced_draw(instances_sm);

        // objects with alpha have to be drawn at the end and in order of depth
        grass.invoke_draw(sm);

        
        // finished drawing to framebuffer, do post processing
        /*
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        screen_texture_sm.activate();
        glClearColor(0.1f, 0.9f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        glDisable(GL_DEPTH_TEST);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, rendered_texture_ID);
        screen_texture_sm.setInt("screenTexture", 0);
        screen_plane.invoke_draw();
        */

        // Instead we just "blit" the FBO with multisampled attachments to the base FBO
        glBindFramebuffer(GL_READ_FRAMEBUFFER, FBO_ID);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        glBlitFramebuffer(0,0,cm.get_view_width(),cm.get_view_height(), 0,0,cm.get_view_width(),cm.get_view_height(), GL_COLOR_BUFFER_BIT, GL_NEAREST);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    std::cout << "Stopped rendering, terminating..." << std::endl;

    glDeleteFramebuffers(1, &FBO_ID);
    glDeleteTextures(1, &MSAA_render_texture_ID);
    glDeleteRenderbuffers(1, &MSAA_RBO_ID);
    glDeleteTextures(1, &skybox_texture_ID);

    glfwTerminate();
    return 0;
}
