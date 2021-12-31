// Copyright Pleep inc. 2021

// std libraries
#include <iostream>
#include <fstream>
#include <string>
#include <algorithm>
#include <queue>

// external
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#define GLM_FORCE_SILENT_WARNINGS
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

#include "animation.h"
#include "animator.h"

CameraManager cm(glm::vec3(0.0f, 0.0f, 4.0f));
CameraManager sun_cm;
CameraManager light_cm;

bool showShadowMap = false;

// TODO: move these
// signature requires int
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // silence warning while not using
    static_cast<void*>(window);

    size_t uWidth  = (size_t)width;
    size_t uHeight = (size_t)height;

    cm.set_view_width(uWidth);
    cm.set_view_height(uHeight);
    // should viewport be controlled by camera manager...? TBD
    glViewport(0, 0, uWidth, uHeight);
}

// signature requires doubles
void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    // silence warning while not using
    static_cast<void*>(window);

    static double lastX = -1;
    static double lastY = -1;
    const float sensitivity = 0.05f;

    float xoffset = lastX == -1.0f ? 0.0f : (float)(xpos - lastX);
    float yoffset = lastY == -1.0f ? 0.0f : (float)(lastY - ypos);

    lastX = xpos;
    lastY = ypos;

    cm.turn_yaw(xoffset * sensitivity);
    cm.turn_pitch(yoffset * sensitivity);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    // silence warning while not using
    static_cast<void*>(window);
    static_cast<void>(xoffset);

    cm.set_view_fov(cm.get_view_fov() - (float)yoffset);
}

void process_input(GLFWwindow *window, double deltaTime)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    {
        glfwSetWindowShouldClose(window, true);
    }

    const float spd = 2.5f * (float)deltaTime;
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

    if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS)
        showShadowMap = !showShadowMap;
}

unsigned int load_gl_cubemap_texture(std::vector<std::string> filenames, bool gammaCorrect = true)
{
    unsigned int texture_id;
    glGenTextures(1, &texture_id);
    glBindTexture(GL_TEXTURE_CUBE_MAP, texture_id);

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
            GLenum internalFormat;
            GLenum dataFormat;
            if (texChannels == 3)
            {
                internalFormat = gammaCorrect ? GL_SRGB : GL_RGB;
                dataFormat = GL_RGB;
            }
            else if (texChannels == 4)
            {
                internalFormat = gammaCorrect ? GL_SRGB_ALPHA : GL_RGBA;
                dataFormat = GL_RGBA;
            }
            else // if (texChannels == 1)
            {
                internalFormat = dataFormat = GL_RED;
            }

            glTexImage2D(
                GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 
                0, internalFormat, texWidth, texHeight, 0, dataFormat, GL_UNSIGNED_BYTE, texData
            );
        }
        else
        {
            std::cerr << "MODEL::load_gl_texture Failed to load texture: " << filenames[i] << std::endl;
        }
        stbi_image_free(texData);
    }

    // clear
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
    return texture_id;
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
    std::cout << view_manager::get_context_ID() << std::endl;
    std::cout << view_manager::get_glfw_version_major() << std::endl;
#endif

    // process cmd options?
    static_cast<void>(argc);

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
    std::vector<Entity> defaultCubes;
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
        defaultCubes.push_back(std::move(cb));
        defaultCubes[i].set_origin(positions[i]);
    }

    Entity lightSource(create_cube_model_ptr());
    lightSource.set_origin(glm::vec3(0.0f, 2.0f, 0.8f));
    lightSource.rotate(glm::radians(55.0f), glm::vec3(1.0, 0.0, 1.0));
    lightSource.set_uniform_scale(0.1f);

    // NOTE: models passed as moved or temporary pointers to give entity exclusive control
    //Entity frog(std::make_unique<Model>("resources/frogWalk.dae"));

    Entity frog;
    frog.set_origin(glm::vec3(0.0f, 0.5f, 0.0f));

    // Example for resetting model afterward
    std::unique_ptr<Model> frogModel = std::make_unique<Model>("resources/vampire/dancing_vampire3.dae");
    Animation frogWalk("resources/vampire/dancing_vampire3.dae", frogModel.get());

    frog.reset_graphics_model(std::move(frogModel));
    // OR
    //frog.reset_graphics_model(create_cube_model_ptr("resources/12268_banjofrog_diffuse.jpg"));

    Animator frogAnimator;
    frogAnimator.play_animation(&frogWalk);


    Entity grass(create_quad_model_ptr("resources/grass.png"));
    grass.set_origin(glm::vec3(0.0f, 1.5f, -1.0f));

    //CollectiveEntity grasses = CollectiveEntity(create_quad_model_ptr("resources/grass.png"));
    CollectiveEntity grasses = CollectiveEntity(create_cube_model_ptr("resources/blending_transparent_window.png"));
    grasses.set_origin(glm::vec3(0.0f, 3.0f, -1.0f));
    grasses.rotate(glm::radians(90.0f), glm::vec3(0,0,1));

    // Fun
    unsigned int numBlades = 10000;
    std::vector<glm::mat4> grassTransforms;
    // lossiness doesn't really matter
    srand((unsigned int)glfwGetTime());
    float radius = 10.0;
    float offset = 3.0f;
    for (unsigned int i = 0; i < numBlades; i++)
    {
        glm::mat4 transform = glm::mat4(1.0f);
        float angle = (float)i / (float)numBlades * 360.0f;
        float displacement = (rand() % (int)(2 * offset * 100)) / 100.0f - offset;
        float x = sin(angle) * radius + displacement;
        displacement = (rand() % (int)(2 * offset * 100)) / 100.0f - offset;
        float y = displacement * 0.4f; // keep height of field smaller compared to width of x and z
        displacement = (rand() % (int)(2 * offset * 100)) / 100.0f - offset;
        float z = cos(angle) * radius + displacement;
        transform = glm::translate(transform, glm::vec3(x, y, z));

        // 2. scale: scale between 0.05 and 0.25f
        float scale = (float)(rand() % 20) / 100.0f + 0.05f;
        transform = glm::scale(transform, glm::vec3(scale));

        // 3. rotation: add random rotation around a (semi)randomly picked rotation axis vector
        float rotAngle = (float)(rand() % 360);
        transform = glm::rotate(transform, rotAngle, glm::vec3(0.4f, 0.6f, 0.8f));

        grassTransforms.push_back(transform);
    }
    grasses.set_instance_transforms(grassTransforms);

    CollectiveEntity floor = CollectiveEntity(create_quad_model_ptr("resources/wall.jpg", "resources/wall_specular.jpg", "resources/toy_box_normal.png", "resources/toy_box_disp.png"));
    floor.set_origin(glm::vec3(0.5f, -0.7f, 0.0));
    floor.rotate(glm::radians(-70.0f), glm::vec3(1.0, 0.0, 0.0));
    std::vector<glm::mat4> floorTransforms;
    for (float i = 0; i < 11; i++)
    {
        for (float j = 0; j < 11; j++)
        {
            glm::mat4 transform = glm::mat4(1.0f);
            transform = glm::translate(transform, glm::vec3(i-5, j-5, 0.0));
            floorTransforms.push_back(transform);
        }
    }
    floor.set_instance_transforms(floorTransforms);

    // manual object with normal map
    Entity brick;
    brick.set_origin(glm::vec3(0.0, 2.5, -0.5));
    brick.set_uniform_scale(2.0);
    brick.reset_graphics_model(create_quad_model_ptr("resources/brickwall.jpg", "resources/brickwall_specular.jpg", "resources/brickwall_normal_up.jpg"));

    Entity bumpy;
    bumpy.set_origin(glm::vec3(2.0, 1.5, 0.5));
    bumpy.set_uniform_scale(1.8f);
    bumpy.rotate(glm::radians(-30.0f), glm::vec3(1.0,1.0,1.0));
    bumpy.reset_graphics_model(create_quad_model_ptr("resources/wood.png", "resources/brickwall_specular.jpg", "resources/toy_box_normal.png", "resources/toy_box_disp.png"));

    // ******************** Shading Objects *************************
    
    glm::vec3 staticColor(10.0f, 10.0f, 10.0f);
    glm::vec3 spotColor(0.6f, 0.8f, 1.0f);

    // use ShaderManager to build shader program from filenames
    ShaderManager sm(
        "source/shaders/tangents_ubo.vs",
        "source/shaders/multi_target_hdr.fs"
    );
    sm.activate();
    //sm.set_float("gamma", 2.2f);
    sm.set_int("num_ray_lights", 0);
    sm.set_vec3("rLights[0].direction", glm::vec3(-0.2f, -1.0f, -0.3f)); 
    sm.set_vec3("rLights[0].attenuation", glm::vec3(1.0f, 0.14f, 0.07f));
    sm.set_vec3("rLights[0].ambient",  staticColor * 0.1f);
    sm.set_vec3("rLights[0].diffuse",  staticColor * 1.0f);
    sm.set_vec3("rLights[0].specular", staticColor * 1.0f);

    sm.set_int("num_point_lights", 1);
    sm.set_vec3("pLights[0].position", lightSource.get_origin());
    sm.set_vec3("pLights[0].attenuation", glm::vec3(1.0f, 0.14f, 0.07f));
    sm.set_vec3("pLights[0].ambient",  staticColor * 0.1f);
    sm.set_vec3("pLights[0].diffuse",  staticColor * 1.0f);
    sm.set_vec3("pLights[0].specular", staticColor * 1.0f);

    sm.set_int("num_spot_lights", 0);
    sm.set_vec3("sLights[0].position", cm.get_position());
    sm.set_vec3("sLights[0].direction", cm.get_direction());
    sm.set_vec3("sLights[0].attenuation", glm::vec3(1.0f, 0.14f, 0.07f));
    sm.set_float("sLights[0].innerCos", 0.99f);
    sm.set_float("sLights[0].outerCos", 0.92f);
    sm.set_vec3("sLights[0].ambient",  spotColor * 0.1f);
    sm.set_vec3("sLights[0].diffuse",  spotColor * 1.0f);
    sm.set_vec3("sLights[0].specular", spotColor * 1.0f);

    ShaderManager instances_sm(
        "source/shaders/tangents_instances_ubo.vs",
        "source/shaders/multi_target_hdr.fs"
    );
    
    instances_sm.activate();
    instances_sm.set_int("num_ray_lights", 0);
    instances_sm.set_vec3("rLights[0].direction", glm::vec3(-0.2f, -1.0f, -0.3f)); 
    instances_sm.set_vec3("rLights[0].attenuation", glm::vec3(1.0f, 0.14f, 0.07f));
    instances_sm.set_vec3("rLights[0].ambient",  staticColor * 0.1f);
    instances_sm.set_vec3("rLights[0].diffuse",  staticColor * 1.0f);
    instances_sm.set_vec3("rLights[0].specular", staticColor * 1.0f);

    instances_sm.set_int("num_point_lights", 1);
    instances_sm.set_vec3("pLights[0].position", lightSource.get_origin());
    instances_sm.set_vec3("pLights[0].attenuation", glm::vec3(1.0f, 0.14f, 0.07f));
    instances_sm.set_vec3("pLights[0].ambient",  staticColor * 0.1f);
    instances_sm.set_vec3("pLights[0].diffuse",  staticColor * 1.0f);
    instances_sm.set_vec3("pLights[0].specular", staticColor * 1.0f);

    instances_sm.set_int("num_spot_lights", 0);
    instances_sm.set_vec3("sLights[0].position", cm.get_position());
    instances_sm.set_vec3("sLights[0].direction", cm.get_direction());
    instances_sm.set_vec3("sLights[0].attenuation", glm::vec3(1.0f, 0.14f, 0.07f));
    instances_sm.set_float("sLights[0].innerCos", 0.99f);
    instances_sm.set_float("sLights[0].outerCos", 0.92f);
    instances_sm.set_vec3("sLights[0].ambient",  spotColor * 0.1f);
    instances_sm.set_vec3("sLights[0].diffuse",  spotColor * 1.0f);
    instances_sm.set_vec3("sLights[0].specular", spotColor * 1.0f);

    ShaderManager normalVisualizer_sm(
        "source/shaders/viewspace_normal.vs",
        "source/shaders/vertex_normals.gs",
        "source/shaders/fixed_color.fs"
    );

    ShaderManager light_sm(
        "source/shaders/projection_ubo.vs",
        "source/shaders/hdr_light_source.fs"
    );
    light_sm.activate();
    light_sm.set_vec3("lightColor", staticColor);

    // rendering options
    sm.activate();
    //glEnable(GL_DEPTH_TEST);  // this is managed during render loop (for skybox, etc)
    // culling (with correct index orientation)
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    // wireframe
    //glPolygonMode( GL_FRONT_AND_BACK, GL_LINE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    // enable stencil operations
    glEnable(GL_STENCIL_TEST);

    // skybox cubemap
    ShaderManager skybox_sm(
        "source/shaders/skybox.vs",
        "source/shaders/skybox_gamma.fs"
    );
    // order (as defined by opengl) is right, left, top, bottom, back, front
    std::vector<std::string> skyboxFilenames{
        "resources/lake_skybox/right.jpg",
        "resources/lake_skybox/left.jpg",
        "resources/lake_skybox/top.jpg",
        "resources/lake_skybox/bottom.jpg",
        "resources/lake_skybox/front.jpg",
        "resources/lake_skybox/back.jpg"
    };
    unsigned int skyboxTexture_id = load_gl_cubemap_texture(skyboxFilenames);
    skybox_sm.activate();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxTexture_id);
    skybox_sm.set_int("cube_map", 0);
    SkyboxVertexGroup skybox;

    frog.reset_graphics_model_environment_cubemaps(skyboxTexture_id);
    for (unsigned int i = 0; i < defaultCubes.size(); i++)
    {
        defaultCubes[i].reset_graphics_model_environment_cubemaps(skyboxTexture_id);
    }

    // direcational light shadow (depth) map renderer/generator
    ShaderManager shadowMap_sm(
        "source/shaders/projection_lighting.vs",
        "source/shaders/empty.fs"
    );
    ShaderManager shadowMapInstances_sm(
        "source/shaders/projection_instances.vs",
        "source/shaders/empty.fs"
    );

    glm::vec3 lPos = glm::vec3(4.0f, 20.0f, 6.0f);

    sun_cm.set_use_perspective(false);
    sun_cm.set_position(lPos);
    sun_cm.set_target(glm::vec3(0.0f));
    sun_cm.set_view_fov(20.0);

    // omni(point) light shadow (depth) map renderer/generator
    ShaderManager shadowCubeMap_sm(
        "source/shaders/basic_transform.vs",
        "source/shaders/cube_map_generator.gs",
        "source/shaders/depth_generator.fs"
    );
    ShaderManager shadowCubeMapInstances_sm(
        "source/shaders/basic_transform_instances.vs",
        "source/shaders/cube_map_generator.gs",
        "source/shaders/depth_generator.fs"
    );

    light_cm.set_position(lightSource.get_origin());
    // we will set other light properties with the shader objects...

    ShaderManager geomBuffer_sm(
        "source/shaders/geom_buffer.vs",
        "source/shaders/geom_buffer.fs"
    );
    ShaderManager geomBufferInstances_sm(
        "source/shaders/geom_buffer_instances.vs",
        "source/shaders/geom_buffer.fs"
    );

    ShaderManager deferredLighting_sm(
        "source/shaders/screen_texture.vs",
        "source/shaders/deferred_lighting_hdr.fs"
    );
    deferredLighting_sm.activate();
    deferredLighting_sm.set_int("num_ray_lights", 0);
    deferredLighting_sm.set_vec3("rLights[0].direction", glm::vec3(-0.2f, -1.0f, -0.3f)); 
    deferredLighting_sm.set_vec3("rLights[0].attenuation", glm::vec3(1.0f, 0.14f, 0.07f));
    deferredLighting_sm.set_vec3("rLights[0].ambient",  staticColor * 0.1f);
    deferredLighting_sm.set_vec3("rLights[0].diffuse",  staticColor * 1.0f);
    deferredLighting_sm.set_vec3("rLights[0].specular", staticColor * 1.0f);

    deferredLighting_sm.set_int("num_point_lights", 1);
    deferredLighting_sm.set_vec3("pLights[0].position", lightSource.get_origin());
    deferredLighting_sm.set_vec3("pLights[0].attenuation", glm::vec3(1.0f, 0.14f, 0.07f));
    deferredLighting_sm.set_vec3("pLights[0].ambient",  staticColor * 0.1f);
    deferredLighting_sm.set_vec3("pLights[0].diffuse",  staticColor * 1.0f);
    deferredLighting_sm.set_vec3("pLights[0].specular", staticColor * 1.0f);

    deferredLighting_sm.set_int("num_spot_lights", 0);


    // Bones to Bananas
    // using foreward rendering for now
    ShaderManager bones_sm(
        "source/shaders/bones_ubo.vs",
        "source/shaders/multi_target_hdr.fs"
    );
    // duplicated lighting assignments... again
    bones_sm.activate();
    bones_sm.set_int("num_ray_lights", 0);
    bones_sm.set_vec3("rLights[0].direction", glm::vec3(-0.2f, -1.0f, -0.3f)); 
    bones_sm.set_vec3("rLights[0].attenuation", glm::vec3(1.0f, 0.14f, 0.07f));
    bones_sm.set_vec3("rLights[0].ambient",  staticColor * 0.1f);
    bones_sm.set_vec3("rLights[0].diffuse",  staticColor * 1.0f);
    bones_sm.set_vec3("rLights[0].specular", staticColor * 1.0f);

    bones_sm.set_int("num_point_lights", 1);
    bones_sm.set_vec3("pLights[0].position", lightSource.get_origin());
    bones_sm.set_vec3("pLights[0].attenuation", glm::vec3(1.0f, 0.14f, 0.07f));
    bones_sm.set_vec3("pLights[0].ambient",  staticColor * 0.1f);
    bones_sm.set_vec3("pLights[0].diffuse",  staticColor * 1.0f);
    bones_sm.set_vec3("pLights[0].specular", staticColor * 1.0f);

    bones_sm.set_int("num_spot_lights", 0);

    // ***************** Uniform buffer for all shaders ************************
      // hold view transforms in a uniform buffer (as all mesh's use the same per frame)
    unsigned int UBO_ID;
    glGenBuffers(1, &UBO_ID);
    glBindBuffer(GL_UNIFORM_BUFFER, UBO_ID);
    glBufferData(GL_UNIFORM_BUFFER, 2 * sizeof(glm::mat4), NULL, GL_STATIC_DRAW);
    // set buffer to bind point 0
    glBindBufferRange(GL_UNIFORM_BUFFER, 0, UBO_ID, 0, 2 * sizeof(glm::mat4));

    // bind Shader's global uniform block view_transform to 0
    sm.activate();
    glUniformBlockBinding(sm.shaderProgram_id, glGetUniformBlockIndex(sm.shaderProgram_id, "view_transforms"), 0);

    normalVisualizer_sm.activate();
    glUniformBlockBinding(normalVisualizer_sm.shaderProgram_id, glGetUniformBlockIndex(normalVisualizer_sm.shaderProgram_id, "view_transforms"), 0);

    instances_sm.activate();
    glUniformBlockBinding(instances_sm.shaderProgram_id, glGetUniformBlockIndex(instances_sm.shaderProgram_id, "view_transforms"), 0);
 
    light_sm.activate();
    glUniformBlockBinding(light_sm.shaderProgram_id, glGetUniformBlockIndex(light_sm.shaderProgram_id, "view_transforms"), 0);

    geomBuffer_sm.activate();
    glUniformBlockBinding(geomBuffer_sm.shaderProgram_id, glGetUniformBlockIndex(geomBuffer_sm.shaderProgram_id, "view_transforms"), 0);
    
    geomBufferInstances_sm.activate();
    glUniformBlockBinding(geomBufferInstances_sm.shaderProgram_id, glGetUniformBlockIndex(geomBufferInstances_sm.shaderProgram_id, "view_transforms"), 0);
    
    bones_sm.activate();
    glUniformBlockBinding(bones_sm.shaderProgram_id, glGetUniformBlockIndex(bones_sm.shaderProgram_id, "view_transforms"), 0);

    // ****************************** finally framebuffers! ************************************
    ShaderManager screenTexture_sm(
        "source/shaders/screen_texture.vs",
        "source/shaders/screen_texture.fs"
    );
    ShaderManager screenTextureHdr_sm(
        "source/shaders/screen_texture.vs",
        "source/shaders/bloom_texture.fs"
    );
    ShaderManager screenTextureDepth_sm(
        "source/shaders/screen_texture.vs",
        "source/shaders/depth_texture.fs"
    );
    ShaderManager screenTextureBlur_sm(
        "source/shaders/screen_texture.vs",
        "source/shaders/gaussian_1d.fs"
    );
    ScreenPlaneVertexGroup screenPlane;

    unsigned int FBO_ID;
    glGenFramebuffers(1, &FBO_ID);
    glBindFramebuffer(GL_FRAMEBUFFER, FBO_ID);

    // render to a texture... (This is used for color)
    /*
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
    */
    // HDR texture buffer
    unsigned int hdrRenderedTexture_id;
    glGenTextures(1, &hdrRenderedTexture_id);
    glBindTexture(GL_TEXTURE_2D, hdrRenderedTexture_id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, cm.get_view_width(), cm.get_view_height(), 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0); // clear
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, hdrRenderedTexture_id, 0);

    // BLOOM texture buffer
    unsigned int bloomRenderedTexture_id;
    glGenTextures(1, &bloomRenderedTexture_id);
    glBindTexture(GL_TEXTURE_2D, bloomRenderedTexture_id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, cm.get_view_width(), cm.get_view_height(), 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0); // clear
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, bloomRenderedTexture_id, 0);

    // register multiple render targets to this FBO
    unsigned int FBO_attachments[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
    glDrawBuffers(2, FBO_attachments);

    // render to a RenderBuffer (this used for depth/stencil buffer)
    unsigned int RBO_ID;
    glGenRenderbuffers(1, &RBO_ID);
    glBindRenderbuffer(GL_RENDERBUFFER, RBO_ID);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT32, cm.get_view_width(), cm.get_view_height());
    glBindRenderbuffer(GL_RENDERBUFFER, 0); // clear
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, RBO_ID);

    // use a multisample texture instead
    //unsigned int MSAA_render_texture_ID;
    //glGenTextures(1, &MSAA_render_texture_ID);
    //glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, MSAA_render_texture_ID);
    //glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 4 /*samples*/, GL_RGB, cm.get_view_width(), cm.get_view_height(), GL_TRUE);
    //// GL_TRUE -> use identical sample locations and the same number of subsamples for each texel
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    //glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);
    //// attach to framebuffer
    //glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, MSAA_render_texture_ID, 0);

    //// use a multisampled renderbuffer instead
    //unsigned int MSAA_RBO_ID;
    //glGenRenderbuffers(1, &MSAA_RBO_ID);
    //glBindRenderbuffer(GL_RENDERBUFFER, MSAA_RBO_ID);
    //glRenderbufferStorageMultisample(GL_RENDERBUFFER, 4 /*samples*/, GL_DEPTH24_STENCIL8, cm.get_view_width(), cm.get_view_height());
    //glBindRenderbuffer(GL_RENDERBUFFER, 0); // clear
    //glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, MSAA_RBO_ID);
    
    // check frambuffer
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        std::cout << "Screen Framebuffer was not complete, rebinding to default" << std::endl;
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0); // back to the screen


    // pre-post-processing
    unsigned int hotPotatofbo_ids[2];
    unsigned int hotPotatoBuffers[2];
    glGenFramebuffers(2, hotPotatofbo_ids);
    glGenTextures(2, hotPotatoBuffers);
    for (unsigned int i = 0; i < 2; i++)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, hotPotatofbo_ids[i]);
        glBindTexture(GL_TEXTURE_2D, hotPotatoBuffers[i]);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, cm.get_view_width(), cm.get_view_height(), 0, GL_RGBA, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        // prevent wrapping
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, hotPotatoBuffers[i], 0);
    }
    // check frambuffer
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        std::cout << "Hot Potato Framebuffer was not complete, rebinding to default" << std::endl;
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0); // back to the screen

    // ***************** Shadow Map Objects *********************
    // TODO: build encapsulated light objects

    // TODO: build shadow map into optional member of light objects
    unsigned int shadowMapFBO_ID;
    glGenFramebuffers(1, &shadowMapFBO_ID);
    const unsigned int SHADOW_WIDTH = 1024, SHADOW_HEIGHT = 1024;
    sun_cm.set_view_width(SHADOW_WIDTH);
    sun_cm.set_view_height(SHADOW_HEIGHT);

    unsigned int shadowMap_id;
    glGenTextures(1, &shadowMap_id);
    glBindTexture(GL_TEXTURE_2D, shadowMap_id);
    // for variance shadows
    //glTexImage2D(GL_TEXTURE_2D, 0, GL_RG32F, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
    // note we also need a depth attachment if we do this instead
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // always have light outside map
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float shadowBorder[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, shadowBorder);

    // THIS MUST BE SET FOR sampler2DShadow
    // GL_TEXTURE_COMPARE_MODE "Specifies the texture comparison mode for currently bound depth textures"
    // default is GL_NONE on NVIDIA which does something idk?
    // GL_COMPARE_REF_TO_TEXTURE interpolates the 3d coord param in texture() and compares it to the texture
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);

    // we create a texture with GL_DEPTH_COMPONENT instead and *then* we can use dept attachment, duh!
    glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO_ID);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadowMap_id, 0);
    glDrawBuffer(GL_NONE);  // these let us draw without color
    glReadBuffer(GL_NONE);  // so the fbo doesn't think its incomplete
    
    // check frambuffer
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        std::cout << "Shadow Framebuffer was not complete, rebinding to default" << std::endl;
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // *************** OMnidirectional Shadow Map Objects ******************
    unsigned int shadowCubeMapFBO_ID;
    glGenFramebuffers(1, &shadowCubeMapFBO_ID);
    // dimensions of the cube faces
    const unsigned int SHADOW_CUBE_WIDTH = 1024, SHADOW_CUBE_HEIGHT = 1024;
    light_cm.set_view_width(SHADOW_CUBE_WIDTH);
    light_cm.set_view_height(SHADOW_CUBE_HEIGHT);
    // set 90 degrees so that each square fov fills a cubemap face
    light_cm.set_view_fov(90.0f);
    light_cm.set_far_plane(25.0);

    // 1 transform for each cubemap face
    std::vector<glm::mat4> omniLightTransforms;
    // omni-lookAt matricies use different "UP" vectors for some reason
    // point lights should have a function to generate these...
    {
        omniLightTransforms.push_back(light_cm.get_projection() * 
            glm::lookAt(light_cm.get_position(), light_cm.get_position() + glm::vec3( 1.0, 0.0, 0.0), glm::vec3(0.0,-1.0, 0.0)));
        omniLightTransforms.push_back(light_cm.get_projection() * 
            glm::lookAt(light_cm.get_position(), light_cm.get_position() + glm::vec3(-1.0, 0.0, 0.0), glm::vec3(0.0,-1.0, 0.0)));
            
        omniLightTransforms.push_back(light_cm.get_projection() * 
            glm::lookAt(light_cm.get_position(), light_cm.get_position() + glm::vec3(0.0, 1.0, 0.0), glm::vec3(0.0, 0.0, 1.0)));
        omniLightTransforms.push_back(light_cm.get_projection() * 
            glm::lookAt(light_cm.get_position(), light_cm.get_position() + glm::vec3(0.0,-1.0, 0.0), glm::vec3(0.0, 0.0,-1.0)));
            
        omniLightTransforms.push_back(light_cm.get_projection() * 
            glm::lookAt(light_cm.get_position(), light_cm.get_position() + glm::vec3(0.0, 0.0, 1.0), glm::vec3(0.0,-1.0, 0.0)));
        omniLightTransforms.push_back(light_cm.get_projection() * 
            glm::lookAt(light_cm.get_position(), light_cm.get_position() + glm::vec3(0.0, 0.0,-1.0), glm::vec3(0.0,-1.0, 0.0)));
    }

    unsigned int shadowCubeMap_id;
    glGenTextures(1, &shadowCubeMap_id);
    glBindTexture(GL_TEXTURE_CUBE_MAP, shadowCubeMap_id);
    for (unsigned int i = 0; i < 6; i++)
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_DEPTH_COMPONENT, SHADOW_CUBE_WIDTH, SHADOW_CUBE_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);

    // attach the entire cubemap to the rbo
    glBindFramebuffer(GL_FRAMEBUFFER, shadowCubeMapFBO_ID);
    // note not Texture2D
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, shadowCubeMap_id, 0);
    glDrawBuffer(GL_NONE);  // these let us draw without color
    glReadBuffer(GL_NONE);  // so the fbo doesn't think its incomplete
    
    // check frambuffer
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        std::cout << "cube Shadow Framebuffer was not complete, rebinding to default" << std::endl;
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Deferred shading framebuffer
    unsigned int GBO_ID;
    glGenFramebuffers(1, &GBO_ID);
    glBindFramebuffer(GL_FRAMEBUFFER, GBO_ID);
    unsigned int gPosition_ID, gNormal_ID, gColorSpec_ID;

    glGenTextures(1, &gPosition_ID);
    glBindTexture(GL_TEXTURE_2D, gPosition_ID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, cm.get_view_width(), cm.get_view_height(), 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gPosition_ID, 0);

    glGenTextures(1, &gNormal_ID);
    glBindTexture(GL_TEXTURE_2D, gNormal_ID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, cm.get_view_width(), cm.get_view_height(), 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, gNormal_ID, 0);

    glGenTextures(1, &gColorSpec_ID);
    glBindTexture(GL_TEXTURE_2D, gColorSpec_ID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, cm.get_view_width(), cm.get_view_height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, gColorSpec_ID, 0);

    unsigned int GBO_attachments[3] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
    glDrawBuffers(3, GBO_attachments);

    // still need a depth buffer
    unsigned int gRbo_ID;
    glGenRenderbuffers(1, &gRbo_ID);
    glBindRenderbuffer(GL_RENDERBUFFER, gRbo_ID);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT32, cm.get_view_width(), cm.get_view_height());
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, gRbo_ID);
    
    // check frambuffer
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        std::cout << "Deferred Shading Framebuffer was not complete, rebinding to default" << std::endl;
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    
    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR)
    {
        std::cerr << "GL Error before rendering: ";
        std::cerr << err << std::endl;
    }

    // ******************** Render Loop *************************
    double lastTimeVal = 0;
    // limiting fps
    //unsigned int FPS = 100;
    //double frameTimeDelta = 1.0/FPS;

    // 5 frame average
    std::queue<double> fpsBuffer;
    for (int i = 0; i < 10; i++)
        fpsBuffer.push(0.0);
    double fpsSum = 0.0;
    
    while(!glfwWindowShouldClose(window))
    {
        double timeVal = glfwGetTime();
        double deltaTime = timeVal - lastTimeVal;
        //if (deltaTime < frameTimeDelta) continue;
        fpsSum -= fpsBuffer.front();
        fpsBuffer.pop();
        fpsBuffer.push(1.0/(timeVal - lastTimeVal));
        fpsSum += fpsBuffer.back();
        lastTimeVal = timeVal;

        std::cout << "FPS: " << fpsSum/10 << "\r" << std::flush;

        process_input(window, deltaTime);

        // set uniform buffers for all ubo shaders
        glBindBuffer(GL_UNIFORM_BUFFER, UBO_ID);
        // buffering from stack data here is ok?
        glm::mat4 UBO_world_to_view = cm.get_lookAt();
        glm::mat4 UBO_projection = cm.get_projection();
        glBufferSubData(GL_UNIFORM_BUFFER, 0,                 sizeof(glm::mat4), &UBO_world_to_view);
        // could optimize further by only resetting projection if fov changes
        glBufferSubData(GL_UNIFORM_BUFFER, sizeof(glm::mat4), sizeof(glm::mat4), &UBO_projection);

        // these are tied to fps for now...
        // manual movement
        for (unsigned int i = 0; i < defaultCubes.size(); i++)
        {
            defaultCubes[i].rotate(glm::radians(i / 10.0f), glm::vec3(1.0f, 0.7f, 0.4f));
        }
        // automated movement
        frogAnimator.update_animation(deltaTime);

        // ***** render shadow map first *****
        shadowMap_sm.activate();
        shadowMap_sm.set_mat4("world_to_view", sun_cm.get_lookAt());
        shadowMap_sm.set_mat4("projection", sun_cm.get_projection());
        shadowMapInstances_sm.activate();
        shadowMapInstances_sm.set_mat4("world_to_view", sun_cm.get_lookAt());
        shadowMapInstances_sm.set_mat4("projection", sun_cm.get_projection());

        glViewport(0,0, SHADOW_WIDTH,SHADOW_HEIGHT);
        glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO_ID);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );//| GL_STENCIL_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);
        // TODO: use scene pipeline to render required entities
        for (unsigned int i = 0; i < defaultCubes.size(); i++)
            defaultCubes[i].invoke_draw(shadowMap_sm);
        frog.invoke_draw(shadowMap_sm);
        brick.invoke_draw(shadowMap_sm);
        bumpy.invoke_draw(shadowMap_sm);
        floor.invoke_instanced_draw(shadowMapInstances_sm);
        grasses.invoke_instanced_draw(shadowMapInstances_sm);
        grass.invoke_draw(shadowMap_sm);
        // ***** done rendering shadow *****

        // ***** Render omni shadow map also *****
        shadowCubeMap_sm.activate();
        shadowCubeMap_sm.set_vec3("source_pos", light_cm.get_position());
        shadowCubeMap_sm.set_float("source_far_plane", light_cm.get_far_plane());
        for (int i = 0; i < 6; i ++)
            shadowCubeMap_sm.set_mat4("omniLightTransforms[" + std::to_string(i) + "]", omniLightTransforms[i]);
        shadowCubeMapInstances_sm.activate();
        shadowCubeMapInstances_sm.set_vec3("source_pos", light_cm.get_position());
        shadowCubeMapInstances_sm.set_float("source_far_plane", light_cm.get_far_plane());
        for (int i = 0; i < 6; i ++)
            shadowCubeMapInstances_sm.set_mat4("omniLightTransforms[" + std::to_string(i) + "]", omniLightTransforms[i]);
        
        glViewport(0,0, SHADOW_CUBE_WIDTH,SHADOW_CUBE_HEIGHT);
        glBindFramebuffer(GL_FRAMEBUFFER, shadowCubeMapFBO_ID);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );//| GL_STENCIL_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);

        // TODO: use scene pipeline to render required entities
        for (unsigned int i = 0; i < defaultCubes.size(); i++)
            defaultCubes[i].invoke_draw(shadowCubeMap_sm);
        frog.invoke_draw(shadowCubeMap_sm);
        brick.invoke_draw(shadowCubeMap_sm);
        bumpy.invoke_draw(shadowCubeMap_sm);
        floor.invoke_instanced_draw(shadowCubeMapInstances_sm);
        grasses.invoke_instanced_draw(shadowCubeMapInstances_sm);
        grass.invoke_draw(shadowCubeMap_sm);
        // ***** really done rendering shadow *****

        // ***** test deferred rendering *****
        // NO BLENDING FOR SOLID GEOMETRY
        glDisable(GL_BLEND);
        glBindFramebuffer(GL_FRAMEBUFFER, GBO_ID);
        glViewport(0,0, cm.get_view_width(),cm.get_view_height());
        // AHHHHH MUST CLEAR 0's (black) for GEOMETRY PASS
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);
        geomBuffer_sm.activate();
        geomBuffer_sm.set_vec3("viewPos", cm.get_position());

        for (unsigned int i = 0; i < defaultCubes.size(); i++)
        {
            defaultCubes[i].invoke_draw(geomBuffer_sm);
        }
        //frog.invoke_draw(geomBuffer_sm);
        brick.invoke_draw(geomBuffer_sm);
        bumpy.invoke_draw(geomBuffer_sm);
        grass.invoke_draw(geomBuffer_sm);

        geomBufferInstances_sm.activate();
        geomBufferInstances_sm.set_vec3("viewPos", cm.get_position());
        floor.invoke_instanced_draw(geomBufferInstances_sm);
        grasses.invoke_instanced_draw(geomBufferInstances_sm);


        // deferred lighting check
        glBindFramebuffer(GL_FRAMEBUFFER, FBO_ID);
        glClearColor(0.7f, 0.7f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glDisable(GL_DEPTH_TEST);
        deferredLighting_sm.activate();
        deferredLighting_sm.set_vec3("viewPos", cm.get_position());
        // TODO: texture index must be managed with model/environment textures
        //      check using 10 for now...
        glActiveTexture(GL_TEXTURE10);
        glBindTexture(GL_TEXTURE_2D, shadowMap_id);
        deferredLighting_sm.set_int("shadow_map", 10);
        deferredLighting_sm.set_mat4("light_transform", sun_cm.get_projection() * sun_cm.get_lookAt());
        // TODO: texture index must be managed with model/environment textures
        //      check using 12 for now...
        glActiveTexture(GL_TEXTURE12);
        glBindTexture(GL_TEXTURE_CUBE_MAP, shadowCubeMap_id);
        deferredLighting_sm.set_int("shadow_cube_map", 12);
        deferredLighting_sm.set_float("light_far_plane", light_cm.get_far_plane());

        // set environment map for ALL MATERIALS?
        // can/should we have this part of the g buffers?
        glActiveTexture(GL_TEXTURE13);
        glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxTexture_id);
        deferredLighting_sm.set_int("cube_map", 13);
        deferredLighting_sm.set_bool("use_cube_map", true);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, gPosition_ID);
        deferredLighting_sm.set_int("GPosition", 0);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, gNormal_ID);
        deferredLighting_sm.set_int("GNormal", 1);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, gColorSpec_ID);
        deferredLighting_sm.set_int("GColorSpec", 2);
        screenPlane.invoke_draw();


        //NOTE: if we want to draw to another buffer (ex: FBO_ID, if we weren't doing post-processing)
        //  we would have to blit the depth buffer bit from GBO_ID to FBO_ID (see the deffered-shading page)
        //  it doesn't seem to work for my gpus/fbos
        glEnable(GL_DEPTH_TEST);
        glBindFramebuffer(GL_READ_FRAMEBUFFER, GBO_ID);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, FBO_ID); // write to lighting
        glBlitFramebuffer(
        0, 0, cm.get_view_width(), cm.get_view_height(), 0, 0, cm.get_view_width(), cm.get_view_height(), GL_DEPTH_BUFFER_BIT, GL_NEAREST
        );


        // then do forward rendering passes afterward
        glBindFramebuffer(GL_FRAMEBUFFER, FBO_ID);
        lightSource.invoke_draw(light_sm);

        bones_sm.activate();
        bones_sm.set_vec3("viewPos", cm.get_position());
        // TODO: texture index must be managed with model/environment textures
        //      check using 10 for now...
        glActiveTexture(GL_TEXTURE10);
        glBindTexture(GL_TEXTURE_2D, shadowMap_id);
        bones_sm.set_int("shadow_map", 10);
        bones_sm.set_mat4("light_transform", sun_cm.get_projection() * sun_cm.get_lookAt());
        // TODO: texture index must be managed with model/environment textures
        //      check using 12 for now...
        glActiveTexture(GL_TEXTURE12);
        glBindTexture(GL_TEXTURE_CUBE_MAP, shadowCubeMap_id);
        bones_sm.set_int("shadow_cube_map", 12);
        bones_sm.set_float("light_far_plane", light_cm.get_far_plane());
        std::vector<glm::mat4> boneTransforms = frogAnimator.get_final_bone_matrices();
        for (unsigned int i = 0; i < boneTransforms.size(); i++)
            bones_sm.set_mat4("finalBonesMatrices[" + std::to_string(i) + "]", boneTransforms[i]);
        frog.invoke_draw(bones_sm);

        // we can also do a skybox foreward pass before hdr/bloom post-processing
        glDepthFunc(GL_LEQUAL); // so skybox only passes if there is nothing else in buffer
        skybox_sm.activate();
        // cut out top-left 3x3 matrix (ommiting translation)
        skybox_sm.set_mat4("world_to_view", glm::mat4(glm::mat3(cm.get_lookAt())));
        skybox_sm.set_mat4("projection", cm.get_projection());
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxTexture_id);
        skybox_sm.set_int("cube_map", 0);
        skybox.invoke_draw();
        // reset depth buffer
        glDepthFunc(GL_LESS);


        // FBO_ID's hdr and bloom textures are set
        // proceed to post-processing

        // ***** you deferred WHAT?! *****
        
/*
        // enable framebuffer for post-processing
        glBindFramebuffer(GL_FRAMEBUFFER, FBO_ID);
        glViewport(0,0, cm.get_view_width(),cm.get_view_height());

        glClearColor(0.1f, 0.1f, 0.9f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );//| GL_STENCIL_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);

        // ***** do all shader setup for this frame *****
        sm.activate();
        sm.set_vec3("sLights[0].position", cm.get_position());
        sm.set_vec3("sLights[0].direction", cm.get_direction());
        sm.set_vec3("viewPos", cm.get_position());
        sm.set_mat4("light_transform", sun_cm.get_projection() * sun_cm.get_lookAt());
        sm.set_float("light_far_plane", light_cm.get_far_plane());
        
        // These are in UBO now
        //sm.set_mat4("world_to_view", cm.get_lookAt());
        //sm.set_mat4("projection", cm.get_projection());
        instances_sm.activate();
        instances_sm.set_vec3("sLights[0].position", cm.get_position());
        instances_sm.set_vec3("sLights[0].direction", cm.get_direction());
        instances_sm.set_vec3("viewPos", cm.get_position());
        instances_sm.set_mat4("light_transform", sun_cm.get_projection() * sun_cm.get_lookAt());
        instances_sm.set_float("light_far_plane", light_cm.get_far_plane());
        // ***** shaders ready to be invoked *****

        // ***** Draw regular pass *****
        sm.activate();
        // TODO: texture index must be managed with model/environment textures
        //      check using 10 for now...
        glActiveTexture(GL_TEXTURE10);
        glBindTexture(GL_TEXTURE_2D, shadowMap_id);
        sm.set_int("shadow_map", 10);
        // TODO: texture index must be managed with model/environment textures
        //      check using 12 for now...
        glActiveTexture(GL_TEXTURE12);
        glBindTexture(GL_TEXTURE_CUBE_MAP, shadowCubeMap_id);
        sm.set_int("shadow_cube_map", 12);

        for (unsigned int i = 0; i < defaultCubes.size(); i++)
        {
            defaultCubes[i].invoke_draw(sm);
        }
        frog.invoke_draw(sm);
        //frog.invoke_draw(normalVisualizer_sm);
        brick.invoke_draw(sm);
        bumpy.invoke_draw(sm);

        lightSource.invoke_draw(light_sm);

        instances_sm.activate();
        // TODO: texture index must be managed with model/environment textures
        //      check using 11 for now...
        glActiveTexture(GL_TEXTURE11);
        glBindTexture(GL_TEXTURE_2D, shadowMap_id);
        instances_sm.set_int("shadow_map", 11);
        // TODO: texture index must be managed with model/environment textures
        //      check using 12 for now...
        glActiveTexture(GL_TEXTURE12);
        glBindTexture(GL_TEXTURE_CUBE_MAP, shadowCubeMap_id);
        instances_sm.set_int("shadow_cube_map", 12);
        
        floor.invoke_instanced_draw(instances_sm);

        // skybox drawn before transparent objects
        glDepthFunc(GL_LEQUAL); // so skybox only passes if there is nothing else in buffer
        skybox_sm.activate();
        // cut out top-left 3x3 matrix (ommiting translation)
        skybox_sm.set_mat4("world_to_view", glm::mat4(glm::mat3(cm.get_lookAt())));
        skybox_sm.set_mat4("projection", cm.get_projection());
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxTexture_id);
        skybox_sm.set_int("cube_map", 0);
        skybox.invoke_draw();
        // reset depth buffer
        glDepthFunc(GL_LESS);

        // objects with alpha have to be drawn at the end and in order of depth
        grasses.invoke_instanced_draw(instances_sm);

        sm.activate();
        // TODO: texture index must be managed with model/environment textures
        //      check using 10 for now...
        glActiveTexture(GL_TEXTURE10);
        glBindTexture(GL_TEXTURE_2D, shadowMap_id);
        sm.set_int("shadow_map", 10);
        grass.invoke_draw(sm);

        // ***** finished drawing to framebuffer, do post processing *****
*/
        // ***** pre-post-processing *****
        glEnable(GL_BLEND);
        bool isHorizontalPass = true;
        bool isFirstPass = true;
        unsigned int passes = 6;
        screenTextureBlur_sm.activate();
        screenTextureBlur_sm.set_int("image", 0);
        for (unsigned int i = 0; i < passes; i++)
        {
            // brace yourselves, where we're going we don't need casting
            glBindFramebuffer(GL_FRAMEBUFFER, hotPotatofbo_ids[isHorizontalPass]);
            screenTextureBlur_sm.set_int("horizontal", isHorizontalPass);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, isFirstPass ? bloomRenderedTexture_id : hotPotatoBuffers[!isHorizontalPass]);
            screenPlane.invoke_draw();
            isHorizontalPass = !isHorizontalPass;
            if (isFirstPass)
                isFirstPass = false;
        }

        // ***** all textures prepped for final approach *****
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // draw screen plane
        if (!showShadowMap)
        {
            screenTextureHdr_sm.activate();
            glClearColor(0.1f, 0.9f, 0.1f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);
            glDisable(GL_DEPTH_TEST);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, hdrRenderedTexture_id);
            screenTextureHdr_sm.set_int("screenTexture", 0);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, hotPotatoBuffers[1]);
            screenTextureHdr_sm.set_int("bloomTexture", 1);
            screenPlane.invoke_draw();
        }
        else
        {
            screenTextureDepth_sm.activate();
            glClearColor(0.9f, 0.1f, 0.1f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);
            glDisable(GL_DEPTH_TEST);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, shadowMap_id);
            screenTextureDepth_sm.set_int("depthMap", 0);
            screenPlane.invoke_draw();
        }

        // or for msaa we can skip
        // Instead we just "blit" the FBO with multisampled attachments to the base FBO
        //glBindFramebuffer(GL_READ_FRAMEBUFFER, FBO_ID);
        //glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        //glBlitFramebuffer(0,0,cm.get_view_width(),cm.get_view_height(), 0,0,cm.get_view_width(),cm.get_view_height(), GL_COLOR_BUFFER_BIT, GL_NEAREST);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    std::cout << "Stopped rendering, terminating..." << std::endl;

    // TODO: once actual scene architecure is buiilt we can worry about deletion
    //  its too much of a pain to do for these temporary things...
    glDeleteFramebuffers(1, &FBO_ID);
    //glDeleteTextures(1, &MSAA_render_texture_ID);
    //glDeleteRenderbuffers(1, &MSAA_RBO_ID);
    glDeleteTextures(1, &skyboxTexture_id);

    glfwTerminate();
    return 0;
}
