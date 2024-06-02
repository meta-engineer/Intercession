#ifndef MODEL_MANAGER_FAUX_H
#define MODEL_MANAGER_FAUX_H

//#include "intercession_pch.h"
#include "rendering/model_manager.h"

namespace pleep
{
    // Overrides any methods of ModelManager that would allocate GPU memory,
    // or make any gpu api calls and... doesn't do that
    // assets should still be constructed with information required for serialization
    // such that a _true_ ModelManager could regenerate them.
    class ModelManagerFaux : public ModelManager
    {
    public:
        bool create_material(const std::string& name, const std::unordered_map<TextureType, std::string>& textureDict) override;
        
    protected:
        // Create empty Material
        std::shared_ptr<Material> _build_material(const aiScene* scene, const aiMaterial* material, const std::string& materialName, const std::string& directory) override;
        // Create empty Armature (Armatures have no gpu data so we _could_ safely populate them)
        Armature _build_armature(const aiNode* node) override;
        // Create empty Mesh
        std::shared_ptr<Mesh> _build_mesh(const aiMesh* mesh, const ImportReceipt& receipt) override;
        // Create empty Animation (Animations have no gpu data so we _could_ safely populate them)
        std::shared_ptr<AnimationSkeletal> _build_animation(const aiAnimation *animation) override;

        // Create BasicMeshTypes with only name and filepath
        std::shared_ptr<Mesh> _build_cube_mesh() override;
        std::shared_ptr<Mesh> _build_quad_mesh() override;
        std::shared_ptr<Mesh> _build_screen_mesh() override;
        std::shared_ptr<Mesh> _build_icosahedron_mesh() override;
    };
}

#endif // MODEL_MANAGER_FAUX_H