#ifndef QUAD_MODEL_H
#define QUAD_MODEL_H

#include "model.h"

// Build Model with standard quad buffer data
std::unique_ptr<Model> create_quad_model_ptr(std::string diffuse_path = "", std::string specular_path = "", std::string normal_path = "", std::string displace_path = "")
{
    // generate cube mesh
    std::vector<Vertex>       vertices;
    std::vector<unsigned int> indices;
    std::vector<Texture>      textures;

    float quad_vertices[] = {
        // coordinates         // normal              // texture coordinates    // tangent
        -0.5f,  0.5f,  0.0f,   0.0f,  0.0f,  1.0f,    0.0f,  0.0f,              1.0f,  0.0f,  0.0f,
         0.5f,  0.5f,  0.0f,   0.0f,  0.0f,  1.0f,    1.0f,  0.0f,              1.0f,  0.0f,  0.0f,
        -0.5f, -0.5f,  0.0f,   0.0f,  0.0f,  1.0f,    0.0f,  1.0f,              1.0f,  0.0f,  0.0f,
         0.5f, -0.5f,  0.0f,   0.0f,  0.0f,  1.0f,    1.0f,  1.0f,              1.0f,  0.0f,  0.0f
    };
    // hardcode 3+3+2
    for (unsigned int i = 0; i < sizeof(quad_vertices) / 8; i++)
    {
        vertices.push_back( Vertex{
            glm::vec3(quad_vertices[i * 11 + 0], quad_vertices[i * 11 + 1], quad_vertices[i * 11 + 2]), 
            glm::vec3(quad_vertices[i * 11 + 3], quad_vertices[i * 11 + 4], quad_vertices[i * 11 + 5]), 
            glm::vec2(quad_vertices[i * 11 + 6], quad_vertices[i * 11 + 7]),
            glm::vec3(quad_vertices[i * 11 + 8], quad_vertices[i * 11 + 9], quad_vertices[i * 11 +10])
        } );
    }

    unsigned int quad_indices[] = {
        0,2,1,
        1,2,3,
    };
    for (unsigned int i = 0; i < sizeof(quad_indices); i++)
    {
        indices.push_back(quad_indices[i]);
    }

    // hijack model's texture loading
    // mesh.invoke_draw() will use all black if none exist
    if (!diffuse_path.empty())
    {
        unsigned int diffuse_tex_id = Model::loadGLTexture(diffuse_path);
        textures.push_back(Texture{diffuse_tex_id, TextureType::diffuse_map, ""});
    }
    if (!specular_path.empty())
    {
        unsigned int specular_tex_id = Model::loadGLTexture(specular_path);
        textures.push_back(Texture{specular_tex_id, TextureType::specular_map, ""});
    }
    if (!normal_path.empty())
    {
        // DO NOT GAMMA CORRECT NORMAL MAP
        unsigned int normal_tex_id = Model::loadGLTexture(normal_path, "", false);
        textures.push_back(Texture{normal_tex_id, TextureType::normal_map, ""});
    }
    if (!displace_path.empty())
    {
        // DO NOT GAMMA CORRECT DISPLACE MAP
        unsigned int displace_tex_id = Model::loadGLTexture(displace_path, "", false);
        textures.push_back(Texture{displace_tex_id, TextureType::displace_map, ""});
    }

    Mesh quad_mesh(vertices, indices, textures);
    return std::unique_ptr<Model>(new Model(quad_mesh));
}

#endif // QUAD_MODEL_H