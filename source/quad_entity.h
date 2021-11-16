
#ifndef QUAD_ENTITY_H
#define QUAD_ENTITY_H

#include "entity.h"
#include "model.h"
// hardcoded generation of cube into entity class

class QuadEntity: public Entity
{
  public:
    QuadEntity(std::string diffuse_path = "", std::string specular_path = "");
};

QuadEntity::QuadEntity(std::string diffuse_path, std::string specular_path)
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

    Mesh quad_mesh(vertices, indices, textures);

    // build my graphics_model
    graphical_model.reset(new Model(quad_mesh));
}


#endif // QUAD_ENTITY_H