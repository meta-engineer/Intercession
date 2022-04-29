#ifndef RENDER_SYNCHRO_H
#define RENDER_SYNCHRO_H

//#include "intercession_pch.h"

#include "ecs/i_synchro.h"
#include "rendering/render_dynamo.h"


namespace pleep
{
    class RenderSynchro : public ISynchro
    {
    public:
        // explicitly inherit constructors
        using ISynchro::ISynchro;
        
        // Provide entities of my sign, and registered ones to dynamo
        // exits early if there was no attached dynamo to use
        // THROWS runtime error if m_ownerCosmos is null
        void update() override;

        // synchro needs a RenderDynamo to operate on
        void attach_dynamo(RenderDynamo* contextDynamo);
        
        // synchro can suggest to registry what signature to use from known cosmos
        // returns empty bitset if desired components could not be found
        static Signature get_signature(Cosmos* cosmos);

    private:
        // Register Cosmos/CosmosBuilder setting the entity of the main camera
        void _set_main_camera_handler(Event setCameraEvent);
        
        // handle any entity related resizing (m_mainCamera)
        // dynamo also handles this, making gl calls
        void _resize_handler(Event resizeEvent);
        
        void _resize_main_camera(int width, int height);

        // dynamo provided by cosmos context to invoke on update
        RenderDynamo* m_attachedRenderDynamo = nullptr;


        // Rendering specific data

        // Cosmos should register the current rendering camera
        // synchro will have to fetch data and update Dynamo each frame
        Entity m_mainCamera = NULL_ENTITY;
    };
}

#endif // RENDER_SYNCHRO_H