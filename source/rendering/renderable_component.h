#ifndef RENDERABLE_COMPONENT_H
#define RENDERABLE_COMPONENT_H

//#include "intercession_pch.h"
#include <vector>
#include <memory>

#include "rendering/mesh.h"
#include "rendering/material.h"
#include "rendering/armature.h"
#include "events/message.h"

namespace pleep
{
    // model library imports a set of meshes, each with an associated material (perhaps the same material?)
    // we want the meshes and materials themselves to be const references to the library cache
    // so that entities don't edit it locally (becuase it isn't a local resource)
    // they have to explicitly go to the model library and edit the "original"

    // we then want to associate each mesh with a material
    // so we could have another set for materials with indices in lockstep
    // given only the mesh you can't know which material it is using (or which compnents it is in)
    // you are also not restricted to only use mesh sets from the same "model"
    // though i'm not sure if this is desirable?

    struct RenderableComponent
    {
        // Loses mesh heirarchy?
        std::vector<std::shared_ptr<const Mesh>> meshData;
        // each material corresponds with the mesh at the same index
        // excess materials are ignored, excess meshes reuse the last-most material
        std::vector<std::shared_ptr<const Material>> materials;

        // mutable copy of armature data to store current bone state between frames
        // (and after animation relay has set them for this frame)
        Armature armature;

        // Additional rendering options. should this be part of a material component?

        // TODO: how is this texture managed? init to id = 0?
        //Texture environmentCubemap;

        //bool shadowCaster;
        //bool shadowReceiver;

        // specify shaders/relays?

        // Adjust mesh in local space
        TransformComponent localTransform;

        // used for debugging right now, not serialized/deserialized
        bool highlight = false;
    };

    // Member pointers makes ModelComponent not sharable, so we must override Message serialization
    template<typename T_Msg>
    Message<T_Msg>& operator<<(Message<T_Msg>& msg, const RenderableComponent& data)
    {
        // Pass ModelCache keys AND source filepaths incase it needs to be imported
        // REMEMBER this is a STACK so reverse the order!!!

        // stack armature data first
        msg << data.armature;

        // stack mat data second
        /// TODO: What if material has no sourceFilepath? (created manually)
        size_t numMats = data.materials.size();
        // (remember this must be in reverse order as well)
        for (size_t m = numMats - 1; m < numMats; m--)
        {
            // push texture map for material
            for (auto texturesIt = data.materials[m]->m_textures.begin(); texturesIt != data.materials[m]->m_textures.end(); texturesIt++)
            {
                msg << texturesIt->second.get_source_filepath();
                msg << texturesIt->first;
            }
            size_t numSources = data.materials[m]->m_textures.size();
            
            // push number of sources (0 -> material-level source, or actually empty)
            msg << numSources;

            // push material source, may be empty if material was created in-code
            msg << data.materials[m]->m_sourceFilepath;
            
            // push name string to be received first
            msg << data.materials[m]->m_name;

        }
        // then push number of materials
        msg << numMats;
        
        // stack mesh data third
        size_t numMeshes = data.meshData.size();
        for (size_t m = numMeshes - 1; m < numMeshes; m--)
        {
            // send path string
            msg << data.meshData[m]->m_sourceFilepath;

            // send name string
            msg << data.meshData[m]->m_name;
        }
        // then push "number" of meshes
        msg << numMeshes;

        // last push extra data
        // transform
        msg << data.localTransform;
        // skip highlight? Only set on client, so server will override

        return msg;
    }
    template<typename T_Msg>
    Message<T_Msg>& operator>>(Message<T_Msg>& msg, RenderableComponent& data)
    {
        // First pop extra data
        // transform
        msg >> data.localTransform;


        // Stream out Meshes
        size_t numMeshes = 0;
        msg >> numMeshes;
        // if no meshes in msg, than clear component
        if (numMeshes == 0)
        {
            data.meshData.clear();
        }
        // if mats deserialized is less than exist in data.materials, clear the excess
        else if (numMeshes < data.meshData.size())
        {
            data.meshData.erase(data.meshData.begin()+numMeshes, data.meshData.end());
        }
        else
        {
            for (size_t m = 0; m < numMeshes; m++)
            {
                // extract mesh name
                std::string newMeshName;
                msg >> newMeshName;

                // extract mesh path
                std::string newMeshPath;
                msg >> newMeshPath;

                // ensure index exists
                while (data.meshData.size() <= m)
                {
                    data.meshData.push_back(nullptr);
                }

                // if msg has no mesh (empty string) also clear component mesh
                if (newMeshName == "")
                {
                    data.meshData[m] = nullptr;
                }
                // if (msg has a mesh and) component either has no mesh OR a different one
                // then fetch from library
                else if (data.meshData[m] == nullptr || data.meshData[m]->m_name != newMeshName)
                {
                    std::shared_ptr<const Mesh> libMesh = ModelCache::fetch_mesh(newMeshName);

                    // check if fetch was not successful
                    if (libMesh == nullptr)
                    {
                        // try to import file
                        ModelCache::ImportReceipt meshReceipt = ModelCache::import(newMeshPath);

                        // confirm the msg mesh name was imported
                        if (std::find(meshReceipt.meshNames.begin(), meshReceipt.meshNames.end(), newMeshName) != meshReceipt.meshNames.end())
                        {
                            // try to fetch again
                            libMesh = ModelCache::fetch_mesh(newMeshName);
                        }
                    }

                    // if import was empty or failed it will be nullptr, assign either way
                    data.meshData[m] = libMesh;
                }
                // else names match, continue as-is
            }
        }

        // Stream out Materials
        size_t numMats = 0;
        msg >> numMats;
        // if msg mats is 0 then clear our mats
        if (numMats == 0)
        {
            data.materials.clear();
        }
        // if mats deserialized is less than exist in data.materials, clear the excess
        else if (numMats < data.materials.size())
        {
            data.materials.erase(data.materials.begin()+numMats, data.materials.end());
        }

        for (size_t m = 0; m < numMats; m++)
        {
            // extract material name
            std::string newMaterialName;
            msg >> newMaterialName;

            // check if material already exists early
            std::shared_ptr<const Material> libMat = ModelCache::fetch_material(newMaterialName);
            
            // extract (or create) material path
            std::string newMaterialPath = "";
            msg >> newMaterialPath;
            //PLEEPLOG_DEBUG("Unpacked material: " + newMaterialName + " with path: " + newMaterialPath);

            // extract number of texture sources
            size_t numSources = 0;
            msg >> numSources;

            // 0 indicates material-level sources
            if (numSources == 0)
            {
                //PLEEPLOG_DEBUG("Found material level source.");
            }
            else
            {
                //PLEEPLOG_DEBUG("Unpacking material: " + newMaterialName + " with " + std::to_string(numSources) + " textures");
                // extract all data to ensure message is cleared
                std::unordered_map<TextureType, std::string> newTextures;
                for (size_t i = 0; i < numSources; i++)
                {
                    TextureType newTextureType;
                    std::string newTexturePath;
                    msg >> newTextureType;
                    msg >> newTexturePath;

                    newTextures.insert({ newTextureType, newTexturePath });
                }

                // check if newMaterialName was already found, if not create new material and then refetch libMat
                if (libMat == nullptr && newMaterialName != "")
                {
                    ModelCache::create_material(newMaterialName, newTextures);
                    libMat = ModelCache::fetch_material(newMaterialName);
                }
            }


            // check if component's material at this index has different mat (or none)
            // (m starts at 0, so we should only ever be 1 index above current materials size)
            if (m >= data.materials.size()
                || data.materials[m] == nullptr
                || data.materials[m]->m_name != newMaterialName)
            {
                // check if early fetch was not successful
                if (libMat == nullptr)
                {
                    // try to import file
                    ModelCache::ImportReceipt materialReceipt = ModelCache::import(newMaterialPath);
                    
                    // confirm the msg material name was imported
                    if (std::find(materialReceipt.materialNames.begin(), materialReceipt.materialNames.end(), newMaterialName) != materialReceipt.materialNames.end())
                    {
                        //try to fetch again
                        libMat = ModelCache::fetch_material(newMaterialName);
                    }
                }

                // if material was empty or failed it will be nullptr, insert anyway to maintain ordering
                if (m >= data.materials.size()) data.materials.push_back(libMat);
                else data.materials[m] = libMat;
            }
            // else material names match, continue as-is
        }

        // last stream out armature
        msg >> data.armature;

        return msg;
    }
}

#endif // RENDERABLE_COMPONENT_H