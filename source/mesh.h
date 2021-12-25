
#ifndef MESH_H
#define MESH_H

#include <iostream>
#include <vector>
#include <string>

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "shader_manager.h"

// Note: struct members are stored sequentially so we can pass them like an array pointer
struct Vertex
{
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 tex_coord;
    glm::vec3 tangent;
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
bool TEXTURE_USE_GAMMA(TextureType t)
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
    Texture                   environment_map;

    Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices, std::vector<Texture> textures);
    // Mesh is not responsible for texture GPU memory! Caller is!
    ~Mesh();

    // Send texture info to shader and draw (with my VAO)
    // Transform matricies/lighting must be sent before this
    void invoke_draw(ShaderManager& sm);
    void invoke_instanced_draw(ShaderManager& sm, size_t amount);

    // set to 0 to ignore environment_map
    void reset_environment_map(unsigned int new_cube_map_id = 0);
    // set attrib pointers for transform matrix starting at attrib location "offset"
    // Instance data Array Buffer MUST be bound already!
    void setup_instance_transform_attrib_array(unsigned int offset);

  private:
    // Array Buffer Object, Vertex Buffer Object, Element Buffer Object
    unsigned int VAO_ID, VBO_ID, EBO_ID;

    // Store struct member data into GL objects and retain IDs
    void setup();

    void set_textures(ShaderManager& sm);
};


Mesh::Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices, std::vector<Texture> textures)
    : vertices(vertices)
    , indices(indices)
    , textures(textures)
{
    setup();
    environment_map.id = 0;
}

Mesh::~Mesh()
{
    glDeleteBuffers(1, &EBO_ID);
    glDeleteBuffers(1, &VBO_ID);
    glDeleteBuffers(1, &VAO_ID);
}

void Mesh::setup()
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
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, tex_coord));
    // vertex tangents
    glEnableVertexAttribArray(3);	
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, tangent));

    // clear binds
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void Mesh::invoke_draw(ShaderManager& sm)
{
    set_textures(sm);

    glBindVertexArray(VAO_ID);
    glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

void Mesh::invoke_instanced_draw(ShaderManager& sm, size_t amount)
{
    if (amount == 0) return;
    set_textures(sm);

    glBindVertexArray(VAO_ID);
    glDrawElementsInstanced(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0, amount);
    glBindVertexArray(0);
}

void Mesh::set_textures(ShaderManager& sm)
{
    // set as many texture units as available
    unsigned int diffuse_index = 0;
    unsigned int specular_index = 0;
    unsigned int normal_index = 0;
    unsigned int displace_index = 0;
    unsigned int i = 0;
    for (; i < textures.size(); i++)
    {
        glActiveTexture(GL_TEXTURE0 + i); // gl addresses can do pointer arithmatic
        glBindTexture(GL_TEXTURE_2D, textures[i].id);

        std::string number;
        switch(textures[i].type)
        {
            case(TextureType::diffuse_map):
                number = std::to_string(diffuse_index++);
                break;
            case(TextureType::specular_map):
                number = std::to_string(specular_index++);
                break;
            case(TextureType::normal_map):
                number = std::to_string(normal_index++);
                sm.setBool("material.set_" + ENUM_TO_STR(textures[i].type) + "_" + number, true);
                break;
            case(TextureType::displace_map):
                number = std::to_string(displace_index++);
                sm.setBool("material.set_" + ENUM_TO_STR(textures[i].type) + "_" + number, true);
                break;
            default:
                std::cerr << "Mesh texture type " << ENUM_TO_STR(textures[i].type) << " not implemented; Ignoring..." << std::endl;
                return;
        }

        // we set all textures as material.TYPE_X
        // so shader will have to use all textures in this form
        sm.setInt("material." + ENUM_TO_STR(textures[i].type) + "_" + number, i);
    }

    // If a model has less than usual textures (i.e.no specular) it will use the last bound one.
    // Instead we'll just use 0 (all black). We could implement a global null texture for visibility
    i++;
    glActiveTexture(GL_TEXTURE0 + i);
    glBindTexture(GL_TEXTURE_2D, 0); // texture_id 0 should be black
    if (diffuse_index == 0) {
        sm.setInt("material." + ENUM_TO_STR(TextureType::diffuse_map) + "_0", i);
    }
    if (specular_index == 0) {
        sm.setInt("material." + ENUM_TO_STR(TextureType::specular_map) + "_0", i);
    }
    if (normal_index == 0) {
        sm.setBool("material.set_" + ENUM_TO_STR(TextureType::normal_map) + "_0", false);
        // sampler data does not ned to be cleared
    }
    if (displace_index == 0) {
        sm.setBool("material.set_" + ENUM_TO_STR(TextureType::displace_map) + "_0", false);
        // sampler data does not ned to be cleared
    }

    // use explicit environment map if id is not default
    if (environment_map.id != 0 && environment_map.type == TextureType::cube_map)
    {
        i++;
        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(GL_TEXTURE_CUBE_MAP, environment_map.id);
        // cube_maps (plural)
        sm.setInt(ENUM_TO_STR(TextureType::cube_map), i);
        sm.setBool("use_cube_map", true);
    }
    else
    {
        // if not bind black texture (0) so NVIDIA is happy
        sm.setInt(ENUM_TO_STR(TextureType::cube_map), i);
        sm.setBool("use_cube_map", false);
    }
}

void Mesh::reset_environment_map(unsigned int new_cube_map_id)
{
    // We don't own the gpu memory. No need to free.
    environment_map.type = TextureType::cube_map;
    environment_map.id = new_cube_map_id;
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