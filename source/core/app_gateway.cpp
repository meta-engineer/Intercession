#include "app_gateway.h"

namespace pleep
{
    AppGateway::AppGateway()
        : m_running(false)
        , m_ctx(nullptr)
        , m_glfwWindow(nullptr)
    {
        this->_build_window_api();

        this->_build_context();
    }
    
    AppGateway::~AppGateway()
    {
        this->_clean_context();

        this->_clean_window_api();
    }

    void AppGateway::run() 
    {
        m_running = true;
        std::cout << "AppGateway run start" << std::endl;

        while (m_running)
        {
            m_ctx->run();

            // handle context closing
            this->stop();
        }

        std::cout << "AppGateway run finish" << std::endl;
    }
    
    void AppGateway::stop() 
    {
        m_running = false;
    }
    
    void AppGateway::reset() 
    {
        this->_clean_context();
        this->_clean_window_api();

        this->_build_window_api();
        this->_build_context();
    }
    
    void AppGateway::_build_window_api()
    {
        assert(!m_glfwWindow);

        std::cout << "AppGateway:: initializing glfw version: ";
        std::cout << glfwGetVersionString() << std::endl;

        glfwInit();
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

        m_glfwWindow = glfwCreateWindow(DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT, DEFAULT_WINDOW_TITLE, NULL, NULL);
        
        if (!m_glfwWindow)
        {
            std::cerr << "AppGateway:: Failed to create GLFW window." << std::endl;
            glfwTerminate();
            assert(false);
        }
        glfwMakeContextCurrent(m_glfwWindow);

        // GLAD manages function pointers for OpenGL, so it must be init before using OpenGL functions
        // inform GLAD of the function to load the address of the OS-specific OpenGL function pointers
        //  (glfw knows them and supplies this function)
        if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
        {
            std::cerr << "Failed to init GLAD" << std::endl;
            assert(false);
        }

        // TODO: this should be determined by the context/cosmos
        // set mouse capture mode
        //glfwSetInputMode(m_glfwWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    }
    
    void AppGateway::_clean_window_api() 
    {
        // what to do if already null?
        assert(m_glfwWindow);

        glfwDestroyWindow(m_glfwWindow);
        m_glfwWindow = nullptr;

        glfwTerminate();
    }
    
    void AppGateway::_build_context() 
    {
        // be strict and LOUD with misuse!
        assert(!m_ctx);
        
        m_ctx = new CosmosContext(m_glfwWindow);
    }
    
    void AppGateway::_clean_context() 
    {
        assert(m_ctx);

        m_ctx->stop();

        delete m_ctx;
        //m_ctx.reset();
    }
}