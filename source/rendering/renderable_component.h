#ifndef RENDERABLE_COMPONENT_H
#define RENDERABLE_COMPONENT_H

//#include "intercession_pch.h"
#include <vector>
#include <memory>

#include "rendering/supermesh.h"
#include "rendering/material.h"
#include "events/message.h"

namespace pleep
{
    // model library imports a set of submeshes, each with an associated material (perhaps the same material?)
    // we want the meshes and materials themselves to be const references to the library cache
    // so that entities don't edit it locally (becuase it isn't a local resource)
    // they have to explicitly go to the model library and edit the "original"

    // we then want to associate each submesh with a material
    // so we could have another set for materials with indices in lockstep
    // given only the submesh you can't know which material it is using (or which compnents it is in)
    // you are also not restricted to only use submesh sets from the same "model"
    // though i'm not sure if this is desirable?

    struct RenderableComponent
    {
        // Loses mesh heirarchy?
        // each supermesh submesh corresponds with the material at the same index
        std::shared_ptr<const Supermesh> meshData = nullptr;
        // each material corresponds with the supermesh submesh at the same index
        // excess materials are ignored, excess submeshes reuse the last-most material
        std::vector<std::shared_ptr<const Material>> materials;
        
        // Additional rendering options. should this be part of a material component?
        // TODO: how is this texture managed? init to id = 0?
        //Texture environmentCubemap;
        //bool shadowCaster;
        //bool shadowReceiver;

        // specify shaders/relays?
    };

    // Member pointers makes ModelComponent not sharable, so we must override Message serialization
    template<typename T_Msg>
    Message<T_Msg>& operator<<(Message<T_Msg>& msg, const RenderableComponent& data)
    {
        // Pass ModelLibrary keys AND source filepaths incase it needs to be imported
        // Need to pass strings as fixed sizes!
        // push "number" of  supermeshes
        size_t numMeshes = data.meshData ? 1 : 0;
        msg << numMeshes;
        if (numMeshes > 0)
        {
            // send name string
            msg << data.meshData->m_name;

            // send path string
            msg << data.meshData->m_sourceFilepath;
        }

        // push number of materials
        size_t numMats = data.materials.size();
        msg << numMats;
        for (size_t m = 0; m < numMats; m++)
        {
            // send name string
            msg << data.materials[m]->m_name;

            // send path string
            msg << data.materials[m]->m_sourceFilepath;
        }
        
        return msg;
    }
    template<typename T_Msg>
    Message<T_Msg>& operator>>(Message<T_Msg>& msg, RenderableComponent& data)
    {
        // Stream out SuperMesh
        size_t numMeshes = 0;
        msg >> numMeshes;
        // if no meshes in msg, than clear component
        if (numMeshes == 0)
        {
            data.meshData = nullptr;
        }
        else
        {
            // extract supermesh name
            std::string newSupermeshName;
            msg >> newSupermeshName;

            // extract supermesh path
            std::string newSupermeshPath;
            msg >> newSupermeshPath;

            // if msg has no mesh (empty string) also clear component mesh
            if (newSupermeshName == "")
            {
                data.meshData = nullptr;
            }
            // if (msg has a mesh and) component either has no mesh OR a different one
            // then fetch from library
            else if (data.meshData == nullptr || data.meshData->m_name != newSupermeshName)
            {
                std::shared_ptr<const Supermesh> libMesh = ModelLibrary::fetch_supermesh(newSupermeshName);

                // check if fetch was not successful
                if (libMesh == nullptr)
                {
                    // try to import file
                    ModelLibrary::ImportReceipt supermeshReceipt = ModelLibrary::import(newSupermeshPath);

                    // confirm the msg supermesh name was imported
                    if (std::find(supermeshReceipt.supermeshNames.begin(), supermeshReceipt.supermeshNames.end(), newSupermeshName) != supermeshReceipt.supermeshNames.end())
                    {
                        // try to fetch again
                        libMesh = ModelLibrary::fetch_supermesh(newSupermeshName);
                    }
                }

                // if supermesh was empty or failed it will be nullptr, assign either way
                data.meshData = libMesh;
            }
            // else names match, continue as-is
        }

        // Stream out Materials
        size_t numMats = 0;
        msg >> numMats;
        // if msg mats is 0 then clear our mats
        if (numMats == 0)
        {
            data.materials.clear();
        }

        for (size_t m = 0; m < numMats; m++)
        {
            // extract material name
            std::string newMaterialName;
            msg >> newMaterialName;

            // extract material path
            std::string newMaterialPath;
            msg >> newMaterialPath;

            // check if component's material at this index has different mat (or none)
            // (m starts at 0, so we should only ever be 1 index above current materials size)
            if (m >= data.materials.size()
                || data.materials[m] == nullptr
                || data.materials[m]->m_name != newMaterialName)
            {
                std::shared_ptr<const Material> libMat = ModelLibrary::fetch_material(newMaterialName);

                // check if fetch was not successful
                if (libMat == nullptr)
                {
                    // try to import file
                    ModelLibrary::ImportReceipt materialReceipt = ModelLibrary::import(newMaterialPath);

                    // confirm the msg material name was imported
                    if (std::find(materialReceipt.materialNames.begin(), materialReceipt.materialNames.end(), newMaterialName) != materialReceipt.materialNames.end())
                    {
                        //try to fetch again
                        libMat = ModelLibrary::fetch_material(newMaterialName);
                    }
                }

                // if material was empty or failed it will be nullptr insert anyway to maintain ordering
                if (m >= data.materials.size()) data.materials.push_back(libMat);
                else data.materials[m] = libMat;
            }
            // else material names match, continue as-is
        }

        return msg;
    }
}

#endif // RENDERABLE_COMPONENT_H