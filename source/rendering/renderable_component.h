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
        PLEEPLOG_DEBUG("Reached unimplemented Message stream in <RenderableComponent> overload!");
        // Pass ModelLibrary keys AND source filepaths incase it needs to be imported
        // Need to pass strings as fixed sizes!
        // should ModelLibrary also limit keys to this size...?
        UNREFERENCED_PARAMETER(data);
        //char fixedSupermeshName[MESSAGE_C_STRING_SIZE] = data.meshData.m_name;
        //msg << data.model ? data.model.get_name() : "";
        
        /* 
        // track current size of body
        uint32_t i = static_cast<uint32_t>(msg.size());

        // resize for data to be pushed
        // does this break the amortized exponential auto-allocating?
        // No, i think resize is ACTUALLY making elements, not just capacity
        msg.body.resize(msg.body.size() + sizeof(T_Data));

        // actually physically copy the data into allocated space
        std::memcpy(msg.body.data() + i, &data, sizeof(T_Data));

        // recalc message size
        msg.header.size = static_cast<uint32_t>(msg.size());
         */
        return msg;
    }
    template<typename T_Msg>
    Message<T_Msg>& operator>>(Message<T_Msg>& msg, RenderableComponent& data)
    {
        PLEEPLOG_DEBUG("Reached unimplemented Message stream out <RenderableComponent> overload!");
        std::string newSupermeshName = "";
        // needs to be fixed length
        //msg >> newFilename;
        // if filenames already match we'll assume model is in lockstep
        if (newSupermeshName != data.meshData->m_name)
        {
            //data.model = ModelLibrary::fetch_model(newFilename);
        }
        // otherwise try to fetch from ModelLibrary

        // otherwise try to import from source filepath
        return msg;
    }
}

#endif // RENDERABLE_COMPONENT_H