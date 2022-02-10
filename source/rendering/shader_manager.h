#ifndef SHADER_MANAGER_H
#define SHADER_MANAGER_H

#include <glad/glad.h>
#define GLM_FORCE_SILENT_WARNINGS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

namespace pleep
{
    class ShaderManager
    {
    public:
        unsigned int shaderProgram_id;

        ShaderManager(const char* vertexPath, const char* fragmentPath);
        ShaderManager(const char* vertexPath, const char* geometryPath, const char* fragmentPath);

        void activate();

        // utility uniform functions
        void set_bool(const std::string &name, bool value) const;  
        void set_int(const std::string &name, int value) const;   
        void set_float(const std::string &name, float value) const;
        void set_mat4(const std::string &name, glm::mat4 matrix) const;
        void set_vec3(const std::string &name, glm::vec3 vec) const;

    private:
        // read and return as string, check for stream errors
        std::string _load_shader_file(const char* filePath);
        // create, compile and return gl object id (or 0 if compilation fails)
        unsigned int _build_vertex_shader(const char* src);
        unsigned int _build_geometry_shader(const char* src);
        unsigned int _build_fragment_shader(const char* src);
        unsigned int _build_shader_program(const unsigned int* shaderIds, const unsigned int numShaders);
    };
}

#endif // SHADER_MANAGER_H