#include "model_manager_faux.h"

namespace pleep
{   
    bool ModelManagerFaux::create_material(const std::string& name, const std::unordered_map<TextureType, std::string>& textureDict) 
    {
        UNREFERENCED_PARAMETER(textureDict);

        auto materialIt = this->m_materialMap.find(name);
        if (materialIt != this->m_materialMap.end())
        {
            PLEEPLOG_WARN("Could not create material " + name + " because that name is already taken");
            return false;
        }

        // Material will have no textures
        std::shared_ptr<Material> newMat = std::make_shared<Material>();
        newMat->m_name = name;
        this->m_materialMap[name] = newMat;
        return true;
    }
    
    std::shared_ptr<Material> ModelManagerFaux::_build_material(const aiMaterial* material, const std::string& materialName, const std::string& directory) 
    {
        UNREFERENCED_PARAMETER(material);
        UNREFERENCED_PARAMETER(materialName);
        UNREFERENCED_PARAMETER(directory);

        // Material will have no textures
        return std::make_shared<Material>();
    }
    
    std::shared_ptr<Armature> ModelManagerFaux::_build_armature(const aiNode* node, const std::string& armatureName) 
    {
        UNREFERENCED_PARAMETER(node);
        UNREFERENCED_PARAMETER(armatureName);
        
        // Armature will have no bones
        return std::make_shared<Armature>();
    }
    
    std::shared_ptr<Mesh> ModelManagerFaux::_build_mesh(const aiMesh* mesh, const ImportReceipt& receipt)
    {
        UNREFERENCED_PARAMETER(mesh);
        UNREFERENCED_PARAMETER(receipt);

        // Mesh will have no vertices & indices, and no bound gpu buffers
        return std::make_shared<Mesh>();
    }
    
    std::shared_ptr<AnimationSkeletal> ModelManagerFaux::_build_animation(const aiAnimation *animation, const std::string& animationName) 
    {
        PLEEPLOG_WARN("NO IMPLEMENTATION FOR Loading animation: " + std::string(animation->mName.C_Str()));
        UNREFERENCED_PARAMETER(animation);
        UNREFERENCED_PARAMETER(animationName);

        // TODO: animations are weird...
        return nullptr;
    }
    
    std::shared_ptr<Supermesh> ModelManagerFaux::_build_cube_supermesh() 
    {
        // Supermesh will have no Meshes
        return std::make_shared<Supermesh>(
            ENUM_TO_STR(BasicSupermeshType::cube),
            ENUM_TO_STR(BasicSupermeshType::cube)
        );
    }
    
    std::shared_ptr<Supermesh> ModelManagerFaux::_build_quad_supermesh() 
    {
        // Supermesh will have no Meshes
        return std::make_shared<Supermesh>(
            ENUM_TO_STR(BasicSupermeshType::quad),
            ENUM_TO_STR(BasicSupermeshType::quad)
        );
    }
    
    std::shared_ptr<Supermesh> ModelManagerFaux::_build_screen_supermesh() 
    {
        // Supermesh will have no Meshes
        return std::make_shared<Supermesh>(
            ENUM_TO_STR(BasicSupermeshType::screen),
            ENUM_TO_STR(BasicSupermeshType::screen)
        );
    }
    
    std::shared_ptr<Supermesh> ModelManagerFaux::_build_icosahedron_supermesh() 
    {
        // Supermesh will have no Meshes
        return std::make_shared<Supermesh>(
            ENUM_TO_STR(BasicSupermeshType::icosahedron),
            ENUM_TO_STR(BasicSupermeshType::icosahedron)
        );
    }
}
