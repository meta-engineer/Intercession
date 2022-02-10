#ifndef MESH_H
#define MESH_H

//#include "intercession_pch.h"
#include <iostream>
#include <vector>
#include <string>

#include <glad/glad.h>
#define GLM_FORCE_SILENT_WARNINGS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "texture.h"
#include "shader_manager.h"

namespace pleep
{
    // lets us store in a vec4 for ease
    #define MAX_BONE_INFLUENCE 4

    // Note: struct members are stored sequentially so we can pass them like an array pointer
    struct Vertex
    {
        glm::vec3 position;
        glm::vec3 normal;
        glm::vec2 texCoord;
        glm::vec3 tangent;

        int boneIds[MAX_BONE_INFLUENCE];
        float boneWeights[MAX_BONE_INFLUENCE];
    };


    // Only stores rendering data, no object data related to world space
    class Mesh
    {
    public:
        std::vector<Vertex>       vertices;
        std::vector<unsigned int> indices;
        std::vector<Texture>      textures;
        // TODO: better texture managing to for different types (once all types are better understood)
        Texture                   environmentCubemap;

        Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices, std::vector<Texture> textures);
        // Mesh is not responsible for texture GPU memory! Caller is!
        ~Mesh();

        // Send texture info to shader and draw (with my VAO)
        // Transform matricies/lighting must be sent before this
        void invoke_draw(ShaderManager& sm);
        void invoke_instanced_draw(ShaderManager& sm, size_t amount);

        // set to 0 to ignore environmentCubemap
        void reset_environment_cubemap(const unsigned int newCubemap_id = 0);
        // set attrib pointers for transform matrix starting at attrib location "offset"
        // Instance data Array Buffer MUST be bound already!
        void setup_instance_transform_attrib_array(unsigned int offset);

    private:
        // Array Buffer Object, Vertex Buffer Object, Element Buffer Object
        unsigned int VAO_ID, VBO_ID, EBO_ID;

        // Store struct member data into GL objects and retain IDs
        void _setup();

        void _set_textures(ShaderManager& sm);
    };
}

#endif // MESH_H