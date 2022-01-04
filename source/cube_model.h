#ifndef CUBE_MODEL_H
#define CUBE_MODEL_H

#include "model.h"

// Build Model with standard quad buffer data
std::unique_ptr<Model> create_cube_model_ptr(std::string diffusePath = "", std::string specularPath = "", std::string normalPath = "")
{
    // generate cube mesh
    std::vector<Vertex>       vertices;
    std::vector<unsigned int> indices;
    std::vector<Texture>      textures;

    // NOTE: tangent should be calculated based on normal and uv (texture coords)
    const float CUBE2_VERTICES[] = {
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
    for (unsigned int i = 0; i < sizeof(CUBE2_VERTICES) / sizeof(float) / 11; i++)
    {
        vertices.push_back( Vertex{
            glm::vec3(CUBE2_VERTICES[i * 11 + 0], CUBE2_VERTICES[i * 11 + 1], CUBE2_VERTICES[i * 11 + 2]), 
            glm::vec3(CUBE2_VERTICES[i * 11 + 3], CUBE2_VERTICES[i * 11 + 4], CUBE2_VERTICES[i * 11 + 5]), 
            glm::vec2(CUBE2_VERTICES[i * 11 + 6], CUBE2_VERTICES[i * 11 + 7]), 
            glm::vec3(CUBE2_VERTICES[i * 11 + 8], CUBE2_VERTICES[i * 11 + 9], CUBE2_VERTICES[i * 11 +10])
        } );

        // Don't forget Bones!
        Model::set_vertex_bone_data_to_default(vertices.back());
    }

    unsigned int CUBE2_INDICES[] = {
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
    for (unsigned int i = 0; i < sizeof(CUBE2_INDICES) / sizeof(unsigned int); i++)
    {
        indices.push_back(CUBE2_INDICES[i]);
    }

    // hijack model's texture loading
    // mesh.invoke_draw() will use all black if none exist
    if (!diffusePath.empty())
    {
        unsigned int diffuseTex_id = Model::load_gl_texture(diffusePath);
        textures.push_back(Texture{diffuseTex_id, TextureType::diffuse_map, ""});
    }
    if (!specularPath.empty())
    {
        unsigned int specularTex_id = Model::load_gl_texture(specularPath);
        textures.push_back(Texture{specularTex_id, TextureType::specular_map, ""});
    }
    if (!normalPath.empty())
    {
        // DO NOT GAMMA CORRECT NORMAL MAP
        unsigned int normalTex_id = Model::load_gl_texture(normalPath, "", false);
        textures.push_back(Texture{normalTex_id, TextureType::normal_map, ""});
    }

    Mesh cubeMesh(vertices, indices, textures);
    return std::unique_ptr<Model>(new Model(cubeMesh));
}

#endif // CUBE_MODEL_H