#ifndef CUBE_MODEL_H
#define CUBE_MODEL_H

#include "model.h"

// Build Model with standard quad buffer data
std::unique_ptr<Model> create_cube_model_ptr(std::string diffuse_path = "", std::string specular_path = "", std::string normal_path = "")
{
    // generate cube mesh
    std::vector<Vertex>       vertices;
    std::vector<unsigned int> indices;
    std::vector<Texture>      textures;

    // NOTE: tangent should be calculated based on normal and uv (texture coords)
    float cube2_vertices[] = {
        // coordinates         // normal              // texture coordinates    // tangent
        -0.5f,  0.5f, -0.5f,   0.0f,  1.0f,  0.0f,    1.5f, -0.5f,              1.0f,  0.0f,  0.0f,
        -0.5f,  0.5f,  0.5f,   0.0f,  1.0f,  0.0f,    1.5f,  1.5f,              1.0f,  0.0f,  0.0f,
         0.5f,  0.5f,  0.5f,   0.0f,  1.0f,  0.0f,   -0.5f,  1.5f,              1.0f,  0.0f,  0.0f,
         0.5f,  0.5f, -0.5f,   0.0f,  1.0f,  0.0f,   -0.5f, -0.5f,              1.0f,  0.0f,  0.0f,   // top

        -0.5f, -0.5f, -0.5f,   0.0f, -1.0f,  0.0f,    1.5f,  1.5f,              1.0f,  0.0f,  0.0f,
        -0.5f, -0.5f,  0.5f,   0.0f, -1.0f,  0.0f,    1.5f, -0.5f,              1.0f,  0.0f,  0.0f,
         0.5f, -0.5f,  0.5f,   0.0f, -1.0f,  0.0f,   -0.5f, -0.5f,              1.0f,  0.0f,  0.0f,
         0.5f, -0.5f, -0.5f,   0.0f, -1.0f,  0.0f,   -0.5f,  1.5f,              1.0f,  0.0f,  0.0f,   // bottom

        -0.5f,  0.5f, -0.5f,  -1.0f,  0.0f,  0.0f,    1.5f, -0.5f,              0.0f,  0.0f,  1.0f,
        -0.5f,  0.5f,  0.5f,  -1.0f,  0.0f,  0.0f,   -0.5f, -0.5f,              0.0f,  0.0f,  1.0f,
        -0.5f, -0.5f, -0.5f,  -1.0f,  0.0f,  0.0f,    1.5f,  1.5f,              0.0f,  0.0f,  1.0f,
        -0.5f, -0.5f,  0.5f,  -1.0f,  0.0f,  0.0f,   -0.5f,  1.5f,              0.0f,  0.0f,  1.0f,   //left

        -0.5f,  0.5f,  0.5f,   0.0f,  0.0f,  1.0f,    1.5f, -0.5f,              1.0f,  0.0f, 0.0f,
         0.5f,  0.5f,  0.5f,   0.0f,  0.0f,  1.0f,   -0.5f, -0.5f,              1.0f,  0.0f, 0.0f,
        -0.5f, -0.5f,  0.5f,   0.0f,  0.0f,  1.0f,    1.5f,  1.5f,              1.0f,  0.0f, 0.0f,
         0.5f, -0.5f,  0.5f,   0.0f,  0.0f,  1.0f,   -0.5f,  1.5f,              1.0f,  0.0f, 0.0f,   // front

         0.5f,  0.5f,  0.5f,   1.0f,  0.0f,  0.0f,    1.5f, -0.5f,              0.0f,  0.0f, -1.0f,
         0.5f,  0.5f, -0.5f,   1.0f,  0.0f,  0.0f,   -0.5f, -0.5f,              0.0f,  0.0f, -1.0f,
         0.5f, -0.5f,  0.5f,   1.0f,  0.0f,  0.0f,    1.5f,  1.5f,              0.0f,  0.0f, -1.0f,
         0.5f, -0.5f, -0.5f,   1.0f,  0.0f,  0.0f,   -0.5f,  1.5f,              0.0f,  0.0f, -1.0f,   // right

        -0.5f,  0.5f, -0.5f,   0.0f,  0.0f, -1.0f,   -0.5f, -0.5f,             -1.0f,  0.0f, 0.0f,
         0.5f,  0.5f, -0.5f,   0.0f,  0.0f, -1.0f,    1.5f, -0.5f,             -1.0f,  0.0f, 0.0f,
        -0.5f, -0.5f, -0.5f,   0.0f,  0.0f, -1.0f,   -0.5f,  1.5f,             -1.0f,  0.0f, 0.0f,
         0.5f, -0.5f, -0.5f,   0.0f,  0.0f, -1.0f,    1.5f,  1.5f,             -1.0f,  0.0f, 0.0f,   // back
    };
    // hardcode 3+3+2+3
    for (unsigned int i = 0; i < sizeof(cube2_vertices) / 8; i++)
    {
        vertices.push_back( Vertex{
            glm::vec3(cube2_vertices[i * 11 + 0], cube2_vertices[i * 11 + 1], cube2_vertices[i * 11 + 2]), 
            glm::vec3(cube2_vertices[i * 11 + 3], cube2_vertices[i * 11 + 4], cube2_vertices[i * 11 + 5]), 
            glm::vec2(cube2_vertices[i * 11 + 6], cube2_vertices[i * 11 + 7]),
            glm::vec3(cube2_vertices[i * 11 + 8], cube2_vertices[i * 11 + 9], cube2_vertices[i * 11 +10])
        } );
    }

    unsigned int cube2_indices[] = {
        0,1,2,
        0,2,3,

        4,6,5,
        4,7,6,

        8,10,9,
        9,10,11,

        12,14,13,
        13,14,15,

        16,18,17,
        17,18,19,

        20,21,23,
        20,23,22
    };
    for (unsigned int i = 0; i < sizeof(cube2_indices); i++)
    {
        indices.push_back(cube2_indices[i]);
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

    Mesh cube_mesh(vertices, indices, textures);
    return std::unique_ptr<Model>(new Model(cube_mesh));
}

#endif // CUBE_MODEL_H