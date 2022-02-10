#include "shader_manager.h"

namespace pleep
{
    ShaderManager::ShaderManager(const char* vertexPath, const char* fragmentPath)
    {
        const std::string VS_STR = ShaderManager::_load_shader_file(vertexPath);
        const std::string FS_STR = ShaderManager::_load_shader_file(fragmentPath);
        const char* VS_SRC = VS_STR.c_str();
        const char* FS_SRC = FS_STR.c_str();

        unsigned int VS_ID = ShaderManager::_build_vertex_shader(VS_SRC);
        unsigned int FS_ID = ShaderManager::_build_fragment_shader(FS_SRC);

        const unsigned int SHADER_IDS[] = {VS_ID, FS_ID};
        this->shaderProgram_id = ShaderManager::_build_shader_program(SHADER_IDS, 2);

        // cleanup
        glDeleteShader(VS_ID);
        glDeleteShader(FS_ID);
    }

    ShaderManager::ShaderManager(const char* vertexPath, const char* geometryPath, const char* fragmentPath)
    {
        const std::string VS_STR = ShaderManager::_load_shader_file(vertexPath);
        const std::string GS_STR = ShaderManager::_load_shader_file(geometryPath);
        const std::string FS_STR = ShaderManager::_load_shader_file(fragmentPath);
        const char* VS_SRC = VS_STR.c_str();
        const char* GS_SRC = GS_STR.c_str();
        const char* FS_SRC = FS_STR.c_str();

        unsigned int VS_ID = ShaderManager::_build_vertex_shader(VS_SRC);
        unsigned int GS_ID = ShaderManager::_build_geometry_shader(GS_SRC);
        unsigned int FS_ID = ShaderManager::_build_fragment_shader(FS_SRC);

        const unsigned int SHADER_IDS[] = {VS_ID, GS_ID, FS_ID};
        this->shaderProgram_id = ShaderManager::_build_shader_program(SHADER_IDS, 3);

        // cleanup
        glDeleteShader(VS_ID);
        glDeleteShader(GS_ID);
        glDeleteShader(FS_ID);
    }

    void ShaderManager::activate()
    {
        glUseProgram(shaderProgram_id);
    }

    void ShaderManager::set_bool(const std::string &name, bool value) const
    {         
        glUniform1i(glGetUniformLocation(shaderProgram_id, name.c_str()), (int)value); 
    }
    void ShaderManager::set_int(const std::string &name, int value) const
    { 
        glUniform1i(glGetUniformLocation(shaderProgram_id, name.c_str()), value); 
    }
    void ShaderManager::set_float(const std::string &name, float value) const
    { 
        glUniform1f(glGetUniformLocation(shaderProgram_id, name.c_str()), value); 
    }
    void ShaderManager::set_mat4(const std::string &name, glm::mat4 matrix) const
    {
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram_id, name.c_str()), 1, GL_FALSE, glm::value_ptr(matrix));
    }

    void ShaderManager::set_vec3(const std::string &name, glm::vec3 vec) const
    {
        glUniform3f(glGetUniformLocation(shaderProgram_id, name.c_str()), vec.x, vec.y, vec.z);
    }


    std::string ShaderManager::_load_shader_file(const char* filePath)
    {
        std::string shaderString;
        std::ifstream shaderFile;
        // ensure ifstream objects can throw exceptions:
        shaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);

        try
        {
            shaderFile.open(filePath);
            std::stringstream shaderStream;
            shaderStream << shaderFile.rdbuf();
            shaderFile.close();
            shaderString = shaderStream.str();
        }
        catch(const std::exception& e)
        {
            std::cerr << "ERROR::SHADER::FILE_NOT_SUCCESSFULLY_READ" << std::endl;
            std::cerr << filePath << std::endl;
            std::cerr << e.what() << '\n';
        }
        
        return shaderString;
    }

    unsigned int ShaderManager::_build_vertex_shader(const char* src)
    {
        unsigned int VS_ID = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(VS_ID, 1, &src, NULL);
        glCompileShader(VS_ID);

        // check compile errors
        int success;
        char infoLog[512];
        glGetShaderiv(VS_ID, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            glGetShaderInfoLog(VS_ID, 512, NULL, infoLog);
            std::cerr << "Vertex shader compilation failed\n" << infoLog << std::endl;
            return 0;
        }
        return VS_ID;
    }

    unsigned int ShaderManager::_build_geometry_shader(const char* src)
    {
        unsigned int GS_ID = glCreateShader(GL_GEOMETRY_SHADER);
        glShaderSource(GS_ID, 1, &src, NULL);
        glCompileShader(GS_ID);

        // check compile errors
        int success;
        char infoLog[512];
        glGetShaderiv(GS_ID, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            glGetShaderInfoLog(GS_ID, 512, NULL, infoLog);
            std::cerr << "Geometry shader compilation failed\n" << infoLog << std::endl;
            return 0;
        }
        return GS_ID;
    }
    
    unsigned int ShaderManager::_build_fragment_shader(const char* src)
    {
        unsigned int FS_ID = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(FS_ID, 1, &src, NULL);
        glCompileShader(FS_ID);

        // check compile errors
        int success;
        char infoLog[512];
        glGetShaderiv(FS_ID, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            glGetShaderInfoLog(FS_ID, 512, NULL, infoLog);
            std::cerr << "Fragment shader compilation failed\n" << infoLog << std::endl;
            return 0;
        }
        return FS_ID;
    }

    unsigned int ShaderManager::_build_shader_program(const unsigned int* shaderIds, const unsigned int numShaders)
    {
        unsigned int newShaderProgram_id = glCreateProgram();
        for (unsigned int i = 0; i < numShaders; ++i)
        {
            glAttachShader(newShaderProgram_id, shaderIds[i]);
        }
        glLinkProgram(newShaderProgram_id);

        //check for linking errors
        int success;
        char infoLog[512];
        glGetProgramiv(newShaderProgram_id, GL_LINK_STATUS, &success);
        if (!success)
        {
            glGetProgramInfoLog(newShaderProgram_id, 512, NULL, infoLog);
            std::cerr << "Shader Program linking failed\n" << infoLog << std::endl;
            return 0;
        }
        return newShaderProgram_id;
    }
}