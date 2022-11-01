#include "i_app_gateway.h"

#include "logging/pleep_log.h"

namespace pleep
{
    void I_AppGateway::_build_window_api()
    {
        assert(!m_windowApi);

        PLEEPLOG_INFO("Initializing glfw version: " + std::string(glfwGetVersionString()));

        glfwInit();
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

        m_windowApi = glfwCreateWindow(DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT, DEFAULT_WINDOW_TITLE, NULL, NULL);
        
        if (!m_windowApi)
        {
            PLEEPLOG_ERROR("Failed to create GLFW window.");
            glfwTerminate();
            assert(false);
        }
        glfwMakeContextCurrent(m_windowApi);

        // GLAD manages function pointers for OpenGL, so it must be init before using OpenGL functions
        // inform GLAD of the function to load the address of the OS-specific OpenGL function pointers
        //  (glfw knows them and supplies this function)
        if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
        {
            PLEEPLOG_ERROR("Failed to init GLAD.");
            assert(false);
        }

        glViewport(0,0, DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT);

        // ui can be tied to windowApi
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO(); (void)io;
        // since when has this implicit concatination existed?
        io.IniFilename = (CONFIG_DIRECTORY "imgui.ini");
        //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
        //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

        // Setup Platform/Renderer specific backends
        ImGui_ImplGlfw_InitForOpenGL(m_windowApi, true);
        const char* glsl_version = "#version 330";
        ImGui_ImplOpenGL3_Init(glsl_version);
        
        // default imgui style
        ImGui::StyleColorsClassic();
    }
    
    void I_AppGateway::_clean_window_api() 
    {
        // what to do if already null?
        assert(m_windowApi);

        // shutdown ui layer on window
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();

        glfwDestroyWindow(m_windowApi);
        m_windowApi = nullptr;

        glfwTerminate();
    }
}