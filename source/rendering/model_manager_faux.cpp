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
        
        // Created material needs to serialize each texture source since there is no single material source file (.mtl)
        // create Textures with type none, so that gpu memory is not allocated
        // but keep correct type in the material's map
        std::unordered_map<TextureType, Texture> emptyTextures;
        for (auto sourceIt = textureDict.begin(); sourceIt != textureDict.end(); sourceIt++)
        {
            emptyTextures.emplace(
                std::piecewise_construct,
                std::forward_as_tuple(static_cast<TextureType>(sourceIt->first)),
                std::forward_as_tuple(TextureType::none, sourceIt->second)
            );
        }

        std::shared_ptr<Material> newMat = std::make_shared<Material>(std::move(emptyTextures));
        newMat->m_name = name;
        this->m_materialMap[name] = newMat;
        return true;
    }
    
    std::shared_ptr<Material> ModelManagerFaux::_build_material(const aiScene* scene, const aiMaterial* material, const std::string& materialName, const std::string& directory) 
    {
        UNREFERENCED_PARAMETER(scene);
        UNREFERENCED_PARAMETER(material);
        UNREFERENCED_PARAMETER(materialName);
        UNREFERENCED_PARAMETER(directory);

        // Material will have no textures
        return std::make_shared<Material>();
    }
    
    Armature ModelManagerFaux::_build_armature(const aiNode* node)
    {
        UNREFERENCED_PARAMETER(node);
        
        // Armature will have no bones
        return Armature{};
    }
    
    std::shared_ptr<Mesh> ModelManagerFaux::_build_mesh(const aiMesh* mesh, const ImportReceipt& receipt)
    {
        UNREFERENCED_PARAMETER(mesh);
        UNREFERENCED_PARAMETER(receipt);

        // Mesh will have no vertices & indices, and no bound gpu buffers
        return std::make_shared<Mesh>();
    }
    
    std::shared_ptr<AnimationSkeletal> ModelManagerFaux::_build_animation(const aiAnimation *animation) 
    {
        UNREFERENCED_PARAMETER(animation);

        return std::make_shared<AnimationSkeletal>();
    }
    
    std::shared_ptr<Mesh> ModelManagerFaux::_build_cube_mesh() 
    {
        // Mesh will have no Meshes
        return std::make_shared<Mesh>();
    }
    
    std::shared_ptr<Mesh> ModelManagerFaux::_build_quad_mesh() 
    {
        // Mesh will have no Meshes
        return std::make_shared<Mesh>();
    }
    
    std::shared_ptr<Mesh> ModelManagerFaux::_build_screen_mesh() 
    {
        // Mesh will have no Meshes
        return std::make_shared<Mesh>();
    }
    
    std::shared_ptr<Mesh> ModelManagerFaux::_build_icosahedron_mesh() 
    {
        // Mesh will have no Meshes
        return std::make_shared<Mesh>();
    }
}
