
#ifndef SHADER_MANAGER_H
#define SHADER_MANAGER_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

class ShaderManager
{
  public:
    unsigned int SP_ID; // Shader Program ID

    ShaderManager(const char* vertexPath, const char* fragmentPath);
    ShaderManager(const char* vertexPath, const char* geometryPath, const char* fragmentPath);

    void activate();

    // utility uniform functions
    void setBool(const std::string &name, bool value) const;  
    void setInt(const std::string &name, int value) const;   
    void setFloat(const std::string &name, float value) const;
    void setMat4(const std::string &name, glm::mat4 matrix) const;
    void setVec3(const std::string &name, glm::vec3 vec) const;

  private:
    // read and return as string, check for stream errors
    std::string load_shader_file(const char* filePath);
    // create, compile and return gl object id (or 0 if compilation fails)
    unsigned int build_vertex_shader(const char* src);
    unsigned int build_geometry_shader(const char* src);
    unsigned int build_fragment_shader(const char* src);
    unsigned int build_shader_program(unsigned int* Shader_IDs, unsigned int num_shaders);
};

ShaderManager::ShaderManager(const char* vertexPath, const char* fragmentPath)
{
    std::string VS_str = ShaderManager::load_shader_file(vertexPath);
    std::string FS_str = ShaderManager::load_shader_file(fragmentPath);
    const char* VS_src = VS_str.c_str();
    const char* FS_src = FS_str.c_str();

    unsigned int VS_ID = ShaderManager::build_vertex_shader(VS_src);
    unsigned int FS_ID = ShaderManager::build_fragment_shader(FS_src);

    unsigned int Shader_IDs[] = {VS_ID, FS_ID};
    this->SP_ID = ShaderManager::build_shader_program(Shader_IDs, 2);

    // cleanup
    glDeleteShader(VS_ID);
    glDeleteShader(FS_ID);
}

ShaderManager::ShaderManager(const char* vertexPath, const char* geometryPath, const char* fragmentPath)
{
    std::string VS_str = ShaderManager::load_shader_file(vertexPath);
    std::string GS_str = ShaderManager::load_shader_file(geometryPath);
    std::string FS_str = ShaderManager::load_shader_file(fragmentPath);
    const char* VS_src = VS_str.c_str();
    const char* GS_src = GS_str.c_str();
    const char* FS_src = FS_str.c_str();

    unsigned int VS_ID = ShaderManager::build_vertex_shader(VS_src);
    unsigned int GS_ID = ShaderManager::build_geometry_shader(GS_src);
    unsigned int FS_ID = ShaderManager::build_fragment_shader(FS_src);

    unsigned int Shader_IDs[] = {VS_ID, GS_ID, FS_ID};
    this->SP_ID = ShaderManager::build_shader_program(Shader_IDs, 3);

    // cleanup
    glDeleteShader(VS_ID);
    glDeleteShader(GS_ID);
    glDeleteShader(FS_ID);
}

void ShaderManager::activate()
{
    glUseProgram(SP_ID);
}

void ShaderManager::setBool(const std::string &name, bool value) const
{         
    glUniform1i(glGetUniformLocation(SP_ID, name.c_str()), (int)value); 
}
void ShaderManager::setInt(const std::string &name, int value) const
{ 
    glUniform1i(glGetUniformLocation(SP_ID, name.c_str()), value); 
}
void ShaderManager::setFloat(const std::string &name, float value) const
{ 
    glUniform1f(glGetUniformLocation(SP_ID, name.c_str()), value); 
}
void ShaderManager::setMat4(const std::string &name, glm::mat4 matrix) const
{
    glUniformMatrix4fv(glGetUniformLocation(SP_ID, name.c_str()), 1, GL_FALSE, glm::value_ptr(matrix));
}

void ShaderManager::setVec3(const std::string &name, glm::vec3 vec) const
{
    glUniform3f(glGetUniformLocation(SP_ID, name.c_str()), vec.x, vec.y, vec.z);
}


std::string ShaderManager::load_shader_file(const char* filePath)
{
    std::string shader_string;
    std::ifstream shader_file;
    // ensure ifstream objects can throw exceptions:
    shader_file.exceptions(std::ifstream::failbit | std::ifstream::badbit);

    try
    {
        shader_file.open(filePath);
        std::stringstream shader_stream;
        shader_stream << shader_file.rdbuf();
        shader_file.close();
        shader_string = shader_stream.str();
    }
    catch(const std::exception& e)
    {
        std::cerr << "ERROR::SHADER::FILE_NOT_SUCCESSFULLY_READ" << std::endl;
        std::cerr << e.what() << '\n';
    }
    
    return shader_string;
}

unsigned int ShaderManager::build_vertex_shader(const char* src)
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

unsigned int ShaderManager::build_geometry_shader(const char* src)
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
 
unsigned int ShaderManager::build_fragment_shader(const char* src)
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

unsigned int ShaderManager::build_shader_program(unsigned int* Shader_IDs, unsigned int num_shaders)
{
    unsigned int new_SP_ID = glCreateProgram();
    for (unsigned int i = 0; i < num_shaders; ++i)
    {
        glAttachShader(new_SP_ID, Shader_IDs[i]);
    }
    glLinkProgram(new_SP_ID);

    //check for linking errors
    int success;
    char infoLog[512];
    glGetProgramiv(new_SP_ID, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(new_SP_ID, 512, NULL, infoLog);
        std::cerr << "Shader Program linking failed\n" << infoLog << std::endl;
        return 0;
    }
    return new_SP_ID;
}

#endif // SHADER_MANAGER_H