#ifndef MESH_H
#define MESH_H

#include <iostream>
#include <vector>
#include <string>

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "shader_manager.h"

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
std::string ENUM_TO_STR(TextureType t)
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


Mesh::Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices, std::vector<Texture> textures)
    : vertices(vertices)
    , indices(indices)
    , textures(textures)
{
    _setup();
    environmentCubemap.id = 0;
}

Mesh::~Mesh()
{
    glDeleteBuffers(1, &EBO_ID);
    glDeleteBuffers(1, &VBO_ID);
    glDeleteBuffers(1, &VAO_ID);
}

void Mesh::_setup()
{
    glGenVertexArrays(1, &VAO_ID);
    glBindVertexArray(VAO_ID);

    glGenBuffers(1, &VBO_ID);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_ID);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);

    glGenBuffers(1, &EBO_ID);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO_ID);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    // vertex attribs will always be position, normal, texture from now on so this can be static 3,3,2
    // vertex positions
    glEnableVertexAttribArray(0);	
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    // vertex normals
    glEnableVertexAttribArray(1);	
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
    // vertex texture coords
    glEnableVertexAttribArray(2);	
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoord));
    // vertex tangents
    glEnableVertexAttribArray(3);	
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, tangent));

    // bone ids
    glEnableVertexAttribArray(4);
    glVertexAttribIPointer(4, 4, GL_INT, sizeof(Vertex), (void*)offsetof(Vertex, boneIds));
    // bone weights
    glEnableVertexAttribArray(5);
    glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, boneWeights));

    // clear binds
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void Mesh::invoke_draw(ShaderManager& sm)
{
    _set_textures(sm);

    glBindVertexArray(VAO_ID);
    glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

void Mesh::invoke_instanced_draw(ShaderManager& sm, size_t amount)
{
    if (amount == 0) return;
    _set_textures(sm);

    glBindVertexArray(VAO_ID);
    glDrawElementsInstanced(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0, amount);
    glBindVertexArray(0);
}

void Mesh::_set_textures(ShaderManager& sm)
{
    // set as many texture units as available
    unsigned int diffuseIndex = 0;
    unsigned int specularIndex = 0;
    unsigned int normalIndex = 0;
    unsigned int displaceIndex = 0;
    unsigned int i = 0;
    for (; i < textures.size(); i++)
    {
        glActiveTexture(GL_TEXTURE0 + i); // gl addresses can do pointer arithmatic
        glBindTexture(GL_TEXTURE_2D, textures[i].id);

        std::string number;
        switch(textures[i].type)
        {
            case(TextureType::diffuse_map):
                number = std::to_string(diffuseIndex++);
                break;
            case(TextureType::specular_map):
                number = std::to_string(specularIndex++);
                break;
            case(TextureType::normal_map):
                number = std::to_string(normalIndex++);
                sm.set_bool("material.enable_" + ENUM_TO_STR(textures[i].type) + "_" + number, true);
                break;
            case(TextureType::displace_map):
                number = std::to_string(displaceIndex++);
                sm.set_bool("material.enable_" + ENUM_TO_STR(textures[i].type) + "_" + number, true);
                break;
            default:
                std::cerr << "Mesh texture type " << ENUM_TO_STR(textures[i].type) << " not implemented; Ignoring..." << std::endl;
                return;
        }

        // we set all textures as material.TYPE_X
        // so shader will have to use all textures in this form
        sm.set_int("material." + ENUM_TO_STR(textures[i].type) + "_" + number, i);
    }

    // If a model has less than usual textures (i.e.no specular) it will use the last bound one.
    // Instead we'll just use 0 (all black). We could implement a global null texture for visibility
    i++;
    glActiveTexture(GL_TEXTURE0 + i);
    glBindTexture(GL_TEXTURE_2D, 0); // texture_id 0 should be black
    if (diffuseIndex == 0) {
        sm.set_int("material." + ENUM_TO_STR(TextureType::diffuse_map) + "_0", i);
    }
    if (specularIndex == 0) {
        sm.set_int("material." + ENUM_TO_STR(TextureType::specular_map) + "_0", i);
    }
    if (normalIndex == 0) {
        sm.set_bool("material.enable_" + ENUM_TO_STR(TextureType::normal_map) + "_0", false);
        // sampler data does not ned to be cleared
    }
    if (displaceIndex == 0) {
        sm.set_bool("material.enable_" + ENUM_TO_STR(TextureType::displace_map) + "_0", false);
        // sampler data does not ned to be cleared
    }

    // use explicit environment map if id is not default
    if (environmentCubemap.id != 0 && environmentCubemap.type == TextureType::cube_map)
    {
        i++;
        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(GL_TEXTURE_CUBE_MAP, environmentCubemap.id);
        // cube_maps (plural)
        sm.set_int("environmentCubemap", i);
        sm.set_bool("environmentCubemap_enable", true);
    }
    else
    {
        // if not bind black texture (0) so NVIDIA is happy
        sm.set_int("environmentCubemap", i);
        sm.set_bool("environmentCubemap_enable", false);
    }
}

void Mesh::reset_environment_cubemap(const unsigned int newCubemap_id)
{
    // We don't own the gpu memory. No need to free.
    environmentCubemap.type = TextureType::cube_map;
    environmentCubemap.id = newCubemap_id;
}

void Mesh::setup_instance_transform_attrib_array(unsigned int offset)
{
    glBindVertexArray(VAO_ID);

    // Meshes already use 0(positions), 1(normals), 2(texture coords), 3(tangents)
    //      optionally 4(boneids), 5(boneweights)
    // so offset = 6 (default) should be used
    glEnableVertexAttribArray(offset + 0);
    glVertexAttribPointer(offset + 0, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)0);
    glVertexAttribDivisor(offset + 0, 1);

    glEnableVertexAttribArray(offset + 1);
    glVertexAttribPointer(offset + 1, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(1 * sizeof(glm::vec4)));
    glVertexAttribDivisor(offset + 1, 1);

    glEnableVertexAttribArray(offset + 2);
    glVertexAttribPointer(offset + 2, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(2 * sizeof(glm::vec4)));
    glVertexAttribDivisor(offset + 2, 1);

    glEnableVertexAttribArray(offset + 3);
    glVertexAttribPointer(offset + 3, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(3 * sizeof(glm::vec4)));
    glVertexAttribDivisor(offset + 3, 1);

    glBindVertexArray(0);
}

#endif // MESH_H