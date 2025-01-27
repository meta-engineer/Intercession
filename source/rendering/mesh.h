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

#include "logging/pleep_log.h"
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

        // uses flag id of -1 to disable bone
        inline void set_bone_data_to_default()
        {
            for (unsigned int i = 0; i < MAX_BONE_INFLUENCE; i++)
            {
                boneIds[i] = -1;
                boneWeights[i] = 0.0f;
            }
        }
        // finds next disabled bone, and sets id and weight. Sets nothing if all 4 are valid bones
        inline void set_bone_data(int boneId, float weight)
        {
            if (weight <= 0.0f)
            {
                // ignore bones with 0 weight, just waste a slot
                //PLEEPLOG_WARN("Tried to set boneId " + std::to_string(boneId) + " to weird weight " + std::to_string(weight));
                return;
            }
            for (unsigned int i = 0; i < MAX_BONE_INFLUENCE; i++)
            {
                if (boneIds[i] < 0)
                {
                    boneIds[i] = boneId;
                    boneWeights[i] = weight;
                    break;
                }
            }
        }
    };


    // Only stores rendering data, no object data related to world space
    class Mesh
    {
    public:
        // Init Mesh with no gpu data
        Mesh() = default;
        // Allocate gpu buffers with these vertices and indices
        Mesh(std::vector<Vertex>& vertices, std::vector<unsigned int>& indices);
        // copying a Mesh would mean GPU memory could be freed by the copy
        Mesh(const Mesh&) = delete;
        // Clear my GPU memory from _setup!
        ~Mesh();

        // OpenGL draw (with my VAO)
        // Textures, transform matricies, and lighting must be sent before this
        // TODO: Do I need sm if I can assume using OpenGL functions?
        void invoke_draw(ShaderManager& sm) const;
        void invoke_instanced_draw(ShaderManager& sm, size_t amount) const;

        // set attrib pointers for transform matrix starting at attrib location "offset"
        // Instance data Array Buffer MUST be bound before calling!!!
        void setup_instance_transform_attrib_array(unsigned int offset = 6);

        // Name given for this mesh
        std::string m_name;
        // Filename this mesh was imported from
        std::string m_sourceFilepath;
        
    private:
        // after _setup, buffer object data is set based on these values, so they should be protected
        std::vector<Vertex>       m_vertices;
        std::vector<unsigned int> m_indices;

        // Array Buffer Object, Vertex Buffer Object, Element Buffer Object
        unsigned int VAO_ID, VBO_ID, EBO_ID = 0;
        bool m_isGlSetup = false;


        // Store struct member data into GL objects and retain IDs
        void _setup();
    };
}

#endif // MESH_H