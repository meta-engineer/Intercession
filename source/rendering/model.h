#ifndef MODEL_H
#define MODEL_H

#include <vector>
#include <string>
#include <map>
#include <cassert>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "mesh.h"
#include "assimp_converters.h"

namespace pleep {
    // Models know about bones, but are fed by animators?
    struct BoneInfo
    {
        int id;
        glm::mat4 model_to_bone;
    };

    class Model
    {
    public:
        Model(std::string path);                  // use assimp to load from file
        Model(std::vector<Mesh> meshes);    // pass pre-constructed meshes
        Model(Mesh mesh);                   // to use a single pre-constructed mesh

        // I am responsable for my meshes GPU memory!!
        ~Model();

        // shader must be activated before calling invoke_draw() !!!!
        void invoke_draw(ShaderManager& sm);
        void invoke_instanced_draw(ShaderManager& sm, size_t amount);

        void reset_all_environment_cubemaps(unsigned int newEnvironmentCubemap_id = 0);
        // set all child meshes to have a mat4 attrib from location = offset to offset+4
        void setup_all_instance_transform_attrib_arrays(unsigned int offset);

        std::map<std::string, BoneInfo>& get_bone_info_map();
        int& get_num_bones();

        // uses flag id of -1 to disable bone
        static void set_vertex_bone_data_to_default(Vertex& vertex);
        // finds next disabled bone, and sets id and weight. Sets nothing if all 4 are valid bones
        static void set_vertex_bone_data(Vertex& vertex, int boneId, float weight);
        
    private:
        void _load_model(std::string path);

        // recursively process from scene->mRootNode
        void _process_node(aiNode *node, const aiScene *scene);
        Mesh _process_mesh(aiMesh *mesh, const aiScene *scene);

        std::vector<Texture> _load_material_textures(aiMaterial *mat, aiTextureType type, TextureType tType);

        // iterating through bones, instead of through verticies like in _process_mesh
        // shown to also accept (const aiScene* scene), but is unused?
        void _extract_bone_weight_for_vertices(std::vector<Vertex>& vertices, aiMesh* mesh);
        
        // TODO: store directly in ModelComponent
    public:
        // TODO: heirarchy of meshes is not preserved
        std::vector<Mesh> meshes;
        
    private:
        // maintain location to find associated files
        std::string directory;
        // TODO: move this to model_builder utilities to share textures across models
        std::vector<Texture> texturesLoaded;

        std::map<std::string, BoneInfo> boneInfoMap;
        // initialize to 0 in declaration?
        int boneIdCounter = 0;
    };
}

#endif // MODEL_H