#ifndef MESH_H
#define MESH_H

//#include "intercession_pch.h"
#include <iostream>
#include <vector>
#include <string>

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>


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

    // these will be used as the prefix for shader Material struct members
    enum TextureType
    {
        diffuse_map,
        specular_map,
        cube_map,
        normal_map,
        displace_map
    };
    // TODO: find an elegant was to convert enum to string
    std::string TEXTURETYPE_TO_STR(TextureType t)
    {
        switch(t)
        {
            case (TextureType::diffuse_map):
                return "diffuse_map";
            case (TextureType::specular_map):
                return "specular_map";
            case (TextureType::cube_map):
                return "cube_map";
            case (TextureType::normal_map):
                return "normal_map";
            case (TextureType::displace_map):
                return "displace_map";
            default:
                return "error_unmapped_texture_type";
        }
    }

    // what texture types will get gamma correction on model load
    bool DOES_TEXTURE_USE_GAMMA(TextureType t)
    {
        switch (t)
        {
            case(TextureType::diffuse_map):
                return true;
            case(TextureType::specular_map):
                return true;
            default:
                return false;
        }
    }

    struct Texture
    {
        unsigned int id;
        TextureType type;
        std::string path; // location where file was loaded from
    };

    struct Mesh
    {
    public:
        Mesh();
        // Mesh is not responsible for texture GPU memory! Caller is!
        // but i am responsible for buffer object's GPU memory!
        ~Mesh();

        std::vector<Vertex>       vertices;
        std::vector<unsigned int> indices;
        std::vector<Texture>      textures;

    private:
        // Array Buffer Object, Vertex Buffer Object, Element Buffer Object
        unsigned int VAO_ID, VBO_ID, EBO_ID;
    };
}

#endif // MESH_H