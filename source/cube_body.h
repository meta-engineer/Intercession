
#ifndef CUBE_BODY_H
#define CUBE_BODY_H

#include "body.h"
#include "model.h"
// hardcoded generation of cube into body class

class CubeBody: public Body
{
  public:
    CubeBody();
};

CubeBody::CubeBody()
{
    // generate cube mesh
    std::vector<Vertex>       vertices;
    std::vector<unsigned int> indices;
    std::vector<Texture>      textures;

    float cube2_vertices[] = {
        // coordinates         // normal              // texture coordinates
        -0.5f,  0.5f, -0.5f,   0.0f,  1.0f,  0.0f,    1.5f,  1.5f,
        -0.5f,  0.5f,  0.5f,   0.0f,  1.0f,  0.0f,   -0.5f,  1.5f,
         0.5f,  0.5f,  0.5f,   0.0f,  1.0f,  0.0f,   -0.5f, -0.5f,
         0.5f,  0.5f, -0.5f,   0.0f,  1.0f,  0.0f,    1.5f, -0.5f,   // top

        -0.5f, -0.5f, -0.5f,   0.0f, -1.0f,  0.0f,    1.5f, -0.5f,
        -0.5f, -0.5f,  0.5f,   0.0f, -1.0f,  0.0f,    1.5f,  1.5f,
         0.5f, -0.5f,  0.5f,   0.0f, -1.0f,  0.0f,   -0.5f,  1.5f,
         0.5f, -0.5f, -0.5f,   0.0f, -1.0f,  0.0f,   -0.5f, -0.5f,   // bottom

        -0.5f,  0.5f, -0.5f,  -1.0f,  0.0f,  0.0f,    1.5f,  1.5f,
        -0.5f,  0.5f,  0.5f,  -1.0f,  0.0f,  0.0f,   -0.5f,  1.5f,
        -0.5f, -0.5f, -0.5f,  -1.0f,  0.0f,  0.0f,    1.5f, -0.5f,
        -0.5f, -0.5f,  0.5f,  -1.0f,  0.0f,  0.0f,   -0.5f, -0.5f,   //left

        -0.5f,  0.5f,  0.5f,   0.0f,  0.0f,  1.0f,    1.5f,  1.5f,
         0.5f,  0.5f,  0.5f,   0.0f,  0.0f,  1.0f,   -0.5f,  1.5f,
        -0.5f, -0.5f,  0.5f,   0.0f,  0.0f,  1.0f,    1.5f, -0.5f,
         0.5f, -0.5f,  0.5f,   0.0f,  0.0f,  1.0f,   -0.5f, -0.5f,   // front

         0.5f,  0.5f,  0.5f,   1.0f,  0.0f,  0.0f,    1.5f,  1.5f,
         0.5f,  0.5f, -0.5f,   1.0f,  0.0f,  0.0f,   -0.5f,  1.5f,
         0.5f, -0.5f,  0.5f,   1.0f,  0.0f,  0.0f,    1.5f, -0.5f,
         0.5f, -0.5f, -0.5f,   1.0f,  0.0f,  0.0f,   -0.5f, -0.5f,   // right

        -0.5f,  0.5f, -0.5f,   0.0f,  0.0f, -1.0f,    1.5f,  1.5f,
         0.5f,  0.5f, -0.5f,   0.0f,  0.0f, -1.0f,   -0.5f,  1.5f,
        -0.5f, -0.5f, -0.5f,   0.0f,  0.0f, -1.0f,    1.5f, -0.5f,
         0.5f, -0.5f, -0.5f,   0.0f,  0.0f, -1.0f,   -0.5f, -0.5f,   // back
    };
    // hardcode 3+3+2
    for (unsigned int i = 0; i < sizeof(cube2_vertices) / 8; i++)
    {
        vertices.push_back( Vertex{
            glm::vec3(cube2_vertices[i * 8 + 0], cube2_vertices[i * 8 + 1], cube2_vertices[i * 8 + 2]), 
            glm::vec3(cube2_vertices[i * 8 + 3], cube2_vertices[i * 8 + 4], cube2_vertices[i * 8 + 5]), 
            glm::vec2(cube2_vertices[i * 8 + 6], cube2_vertices[i * 8 + 7])
        } );
    }

    unsigned int cube2_indices[] = {
        0,1,2,
        0,2,3,

        4,5,6,
        4,6,7,

        8,9,10,
        9,10,11,

        12,13,14,
        13,14,15,

        16,17,18,
        17,18,19,

        20,21,23,
        20,22,23
    };
    for (unsigned int i = 0; i < sizeof(cube2_indices); i++)
    {
        indices.push_back(cube2_indices[i]);
    }

    // hijack model's texture loading
    unsigned int tex_diffuse_id = Model::loadGLTexture("container2.png", "resources");
    unsigned int tex_specular_id = Model::loadGLTexture("container2_specular.png", "resources");
    textures.push_back(Texture{tex_diffuse_id, TextureType::diffuse_map, ""});
    textures.push_back(Texture{tex_specular_id, TextureType::specular_map, ""});

    Mesh cube_mesh(vertices, indices, textures);

    // build my graphics_model
    graphical_model.reset(new Model(cube_mesh));
}


#endif // CUBE_BODY_H