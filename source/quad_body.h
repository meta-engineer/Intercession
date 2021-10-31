
#ifndef QUAD_BODY_H
#define QUAD_BODY_H

#include "body.h"
#include "model.h"
// hardcoded generation of cube into body class

class QuadBody: public Body
{
  public:
    QuadBody();
};

QuadBody::QuadBody()
{
    // generate cube mesh
    std::vector<Vertex>       vertices;
    std::vector<unsigned int> indices;
    std::vector<Texture>      textures;

    float quad_vertices[] = {
        // coordinates         // normal              // texture coordinates
        -0.5f,  0.5f,  0.0f,   0.0f,  0.0f,  1.0f,    0.0f,  0.0f,
         0.5f,  0.5f,  0.0f,   0.0f,  0.0f,  1.0f,    1.0f,  0.0f,
        -0.5f, -0.5f,  0.0f,   0.0f,  0.0f,  1.0f,    0.0f,  1.0f,
         0.5f, -0.5f,  0.0f,   0.0f,  0.0f,  1.0f,    1.0f,  1.0f
    };
    // hardcode 3+3+2
    for (unsigned int i = 0; i < sizeof(quad_vertices) / 8; i++)
    {
        vertices.push_back( Vertex{
            glm::vec3(quad_vertices[i * 8 + 0], quad_vertices[i * 8 + 1], quad_vertices[i * 8 + 2]), 
            glm::vec3(quad_vertices[i * 8 + 3], quad_vertices[i * 8 + 4], quad_vertices[i * 8 + 5]), 
            glm::vec2(quad_vertices[i * 8 + 6], quad_vertices[i * 8 + 7])
        } );
    }

    unsigned int quad_indices[] = {
        0,1,2,
        1,2,3,
    };
    for (unsigned int i = 0; i < sizeof(quad_indices); i++)
    {
        indices.push_back(quad_indices[i]);
    }

    // hijack model's texture loading
    unsigned int tex_diffuse_id = Model::loadGLTexture("grass.png", "resources");
    unsigned int tex_specular_id = Model::loadGLTexture("grass.png", "resources");
    textures.push_back(Texture{tex_diffuse_id, TextureType::diffuse_map, ""});
    textures.push_back(Texture{tex_specular_id, TextureType::specular_map, ""});

    Mesh quad_mesh(vertices, indices, textures);

    // build my graphics_model
    graphical_model.reset(new Model(quad_mesh));
}


#endif // QUAD_BODY_H