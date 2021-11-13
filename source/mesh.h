
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
};

// these will be used as the prefix for shader Material struct members
enum TextureType
{
    diffuse_map,
    specular_map,
    cube_map,
    normal_map
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
        default:
            return "error_unmapped_texture_type";
    }
}

struct Texture
{
    unsigned int id;
    TextureType type;
    std::string path; // location where file was loaded from
};

// Only stores rendering data, no object data related to world space
class Mesh {
  public:
    std::vector<Vertex>       vertices;
    std::vector<unsigned int> indices;
    std::vector<Texture>      textures;
    // TODO: better texture managing to avoid blindly using a vector of different types
    Texture                   environment_map;

    Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices, std::vector<Texture> textures);
    ~Mesh();

    // Send texture info to shader and draw (with my VAO)
    // Transform matricies/lighting must be sent before this
    void invoke_draw(ShaderManager& sm);

    // set to 0 to ignore environment_map
    void reset_environment_map(unsigned int new_cube_map_id = 0);

  private:
    unsigned int VAO_ID, VBO_ID, EBO_ID;

    // Store struct member data into GL objects and retain IDs
    void setup();
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

    // TODO: who owns the textures and who owns the environment_map??
    //while (textures.size() > 0)
    //{
    //    glDeleteTextures(1, &(textures.back().id));
    //    std::cout << "Deleting texture " << textures.back().id << std::endl;
    //    textures.pop_back();
    //}
//
    //if (environment_map.id > 0)
    //{
    //    std::cout << "Deleting environment_map " << environment_map.id << std::endl;
    //    glDeleteTextures(1, &(environment_map.id));
    //    environment_map.id = 0;
    //}
}

void Mesh::setup()
{
    glGenVertexArrays(1, &VAO_ID);
    glBindVertexArray(VAO_ID);

    glGenBuffers(1, &VBO_ID);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_ID);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);

    glGenBuffers(1, &EBO_ID);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO_ID);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

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

    // clear binds
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void Mesh::invoke_draw(ShaderManager& sm)
{
    // set as many texture units as available
    unsigned int diffuse_index = 0;
    unsigned int specular_index = 0;
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
            default:
                std::cerr << "Mesh texture type " << ENUM_TO_STR(textures[i].type) << " not implemented; Ignoring..." << std::endl;
                return;
        }

        // we set all textures as material.TYPE_X
        // so shader will have to use all textures in this form
        sm.setInt("material." + ENUM_TO_STR(textures[i].type) + "_" + number, i);
    }

    // TODO: if a model has less than usual textures (no specular) it will use the last bound one. Implement global defaults for visibility (just using 0 here)
    i++;
    glActiveTexture(GL_TEXTURE0 + i);
    glBindTexture(GL_TEXTURE_2D, 0); // texture_id 0 should be black
    if (diffuse_index == 0) {
        sm.setInt("material." + ENUM_TO_STR(TextureType::diffuse_map) + "_0", i);
    }
    if (specular_index == 0) {
        sm.setInt("material." + ENUM_TO_STR(TextureType::specular_map) + "_0", i);
    }

    // use explicit environment map if id is not default
    if (environment_map.id != 0 && environment_map.type == TextureType::cube_map)
    {
        i++;
        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(GL_TEXTURE_CUBE_MAP, environment_map.id);
        sm.setInt(ENUM_TO_STR(TextureType::cube_map), i);
        sm.setInt("num_cube_maps", 1);
    }
    else
    {
        sm.setInt("num_cube_maps", 0);
    }

    glBindVertexArray(VAO_ID);
    glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

void Mesh::reset_environment_map(unsigned int new_cube_map_id)
{
    // TODO: who owns the environment map?
    //if (environment_map.id != 0)
    //{
    //    glDeleteTextures(1, &(environment_map.id));
    //}
    environment_map.type = TextureType::cube_map;
    environment_map.id = new_cube_map_id;
}

#endif // MESH_H