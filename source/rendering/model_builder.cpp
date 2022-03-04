#include "model_builder.h"

namespace pleep
{
    namespace model_builder
    {
        std::shared_ptr<Model> create_cube(std::string diffusePath, std::string specularPath, std::string normalPath, std::string displacePath)
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
            if (!displacePath.empty())
            {
                // DO NOT GAMMA CORRECT DISPLACE MAP
                unsigned int displaceTex_id = Model::load_gl_texture(displacePath, "", false);
                textures.push_back(Texture{displaceTex_id, TextureType::displace_map, ""});
            }

            Mesh cubeMesh(vertices, indices, textures);
            return std::shared_ptr<Model>(new Model(cubeMesh));
        }
        
        std::unique_ptr<Model> create_quad(std::string diffusePath, std::string specularPath, std::string normalPath, std::string displacePath) 
        {
            // generate cube mesh
            std::vector<Vertex>       vertices;
            std::vector<unsigned int> indices;
            std::vector<Texture>      textures;

            const float QUAD_VERTICES[] = {
                // coordinates         // normal              // texture coordinates    // tangent
                -0.5f,  0.5f,  0.0f,   0.0f,  0.0f,  1.0f,    0.0f,  0.0f,              1.0f,  0.0f,  0.0f,
                 0.5f,  0.5f,  0.0f,   0.0f,  0.0f,  1.0f,    1.0f,  0.0f,              1.0f,  0.0f,  0.0f,
                -0.5f, -0.5f,  0.0f,   0.0f,  0.0f,  1.0f,    0.0f,  1.0f,              1.0f,  0.0f,  0.0f,
                 0.5f, -0.5f,  0.0f,   0.0f,  0.0f,  1.0f,    1.0f,  1.0f,              1.0f,  0.0f,  0.0f
            };
            // hardcode 3+3+2
            for (unsigned int i = 0; i < sizeof(QUAD_VERTICES) / sizeof(float) / 11; i++)
            {
                vertices.push_back( Vertex{
                    glm::vec3(QUAD_VERTICES[i * 11 + 0], QUAD_VERTICES[i * 11 + 1], QUAD_VERTICES[i * 11 + 2]), 
                    glm::vec3(QUAD_VERTICES[i * 11 + 3], QUAD_VERTICES[i * 11 + 4], QUAD_VERTICES[i * 11 + 5]), 
                    glm::vec2(QUAD_VERTICES[i * 11 + 6], QUAD_VERTICES[i * 11 + 7]),
                    glm::vec3(QUAD_VERTICES[i * 11 + 8], QUAD_VERTICES[i * 11 + 9], QUAD_VERTICES[i * 11 +10])
                } );
                
                // Don't forget Bones!
                Model::set_vertex_bone_data_to_default(vertices.back());
            }

            const unsigned int QUAD_INDICES[] = {
                0,2,1,
                1,2,3,
            };
            for (unsigned int i = 0; i < sizeof(QUAD_INDICES) / sizeof(unsigned int); i++)
            {
                indices.push_back(QUAD_INDICES[i]);
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
            if (!displacePath.empty())
            {
                // DO NOT GAMMA CORRECT DISPLACE MAP
                unsigned int displaceTex_id = Model::load_gl_texture(displacePath, "", false);
                textures.push_back(Texture{displaceTex_id, TextureType::displace_map, ""});
            }

            Mesh quadMesh(vertices, indices, textures);
            return std::unique_ptr<Model>(new Model(quadMesh));
        }
        
        std::shared_ptr<VertexGroup> create_screen_plane()
        {
            const float SCREEN_PLANE_VERTICES[] = {
                // coordinates       // texture coordinates
                -1.0f,  1.0f,  0.0f,   0.0f,  1.0f,
                1.0f,  1.0f,  0.0f,   1.0f,  1.0f,
                -1.0f, -1.0f,  0.0f,   0.0f, 0.0f,
                1.0f, -1.0f,  0.0f,   1.0f, 0.0f
            };
            const unsigned int SCREEN_PLANE_INDICES[] = {
                0,2,1,
                1,2,3
            };
            const unsigned int SCREEN_PLANE_ATTRIBS[] = {
                3,2
            };
            
            // should load stack data into VAO, then it can be removed when this stack ends
            return std::make_shared<VertexGroup>(
                SCREEN_PLANE_VERTICES, sizeof(SCREEN_PLANE_VERTICES) / sizeof(float), 
                SCREEN_PLANE_INDICES, sizeof(SCREEN_PLANE_INDICES) / sizeof(unsigned int), 
                SCREEN_PLANE_ATTRIBS, sizeof(SCREEN_PLANE_ATTRIBS) / sizeof(unsigned int)
            );
        }
    }
}