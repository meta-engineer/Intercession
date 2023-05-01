#include "render_synchro.h"

#include <exception>

#include "logging/pleep_log.h"
#include "core/cosmos.h"

#include "physics/transform_component.h"
#include "rendering/camera_component.h"
#include "rendering/renderable_component.h"
#include "rendering/render_packet.h"

namespace pleep
{
    void RenderSynchro::update()
    {
        // No owner is a fatal error
        if (m_ownerCosmos == nullptr)
        {
            PLEEPLOG_ERROR("Synchro has no owner Cosmos");
            throw std::runtime_error("Render Synchro started update without owner Cosmos");
        }

        // no dynamo is a mistake, not necessarily an error
        if (m_attachedRenderDynamo == nullptr)
        {
            //PLEEPLOG_WARN("Synchro update was called without an attached Dynamo");
            return;
        }

        // update view info with registered camera
        // re-get camera each time becuase ECS pointers are volatile
        if (m_mainCamera != NULL_ENTITY)
        {
            m_attachedRenderDynamo->submit(CameraPacket {
                m_ownerCosmos->get_component<TransformComponent>(m_mainCamera),
                m_ownerCosmos->get_component<CameraComponent>(m_mainCamera)
            });
        }
        else
        {
            PLEEPLOG_WARN("Update called while no main camera entity was set");
        }

        // feed components of m_entities to attached ControlDynamo
        // I should implicitly know my signature and therefore what components i can fetch
        for (Entity const& entity : m_entities)
        {
            TransformComponent& transform = m_ownerCosmos->get_component<TransformComponent>(entity);
            RenderableComponent& renderable = m_ownerCosmos->get_component<RenderableComponent>(entity);
            
            // catch null supermesh and don't even bother dynamo with it
            if (renderable.meshData == nullptr) continue;

            m_attachedRenderDynamo->submit(RenderPacket{ transform, renderable });
        }

        // CosmosContext will flush dynamo relays once all synchros are done
        // Other components (Context) may draw further and then Context will flush at frame end
    }
    
    Signature RenderSynchro::derive_signature(std::shared_ptr<Cosmos> cosmos) 
    {
        Signature sign;

        try
        {
            sign.set(cosmos->get_component_type<TransformComponent>());
            sign.set(cosmos->get_component_type<RenderableComponent>());
        }
        catch(const std::exception& e)
        {
            // Component Registry already logs error
            UNREFERENCED_PARAMETER(e);
            sign.reset();
            PLEEPLOG_ERROR("Synchro could not get desired component types from cosmos. Have TransformComponent and RenderableComponent been registered?");
        }
        
        return sign;
    }
    
    void RenderSynchro::attach_dynamo(std::shared_ptr<RenderDynamo> contextDynamo) 
    {
        // We cannot subscribe to events until the dynamo is attached (to have broker access)
        // So we have to do it here, but make sure we don't double subscribe if dynamo changes
        if (m_attachedRenderDynamo)
        {
            // events subscribed elsewhere during runtime also need to be here?

            m_attachedRenderDynamo->get_shared_broker()->remove_listener(METHOD_LISTENER(events::rendering::SET_MAIN_CAMERA, RenderSynchro::_set_main_camera_handler));
            
            m_attachedRenderDynamo->get_shared_broker()->remove_listener(METHOD_LISTENER(events::window::RESIZE, RenderSynchro::_resize_handler));
        }

        m_attachedRenderDynamo = contextDynamo;

        if (m_attachedRenderDynamo)
        {
            // remember this is broadcast so if there are more than 1 synchro they will all be set
            m_attachedRenderDynamo->get_shared_broker()->add_listener(METHOD_LISTENER(events::rendering::SET_MAIN_CAMERA, RenderSynchro::_set_main_camera_handler));
            
            // update main camera entity
            m_attachedRenderDynamo->get_shared_broker()->add_listener(METHOD_LISTENER(events::window::RESIZE, RenderSynchro::_resize_handler));
        }
    }
    
    void RenderSynchro::_set_main_camera_handler(EventMessage setCameraEvent) 
    {
        events::rendering::SET_MAIN_CAMERA_params setCameraParams;
        setCameraEvent >> setCameraParams;
        
        PLEEPLOG_TRACE("Handling event " + std::to_string(events::rendering::SET_MAIN_CAMERA) + " (events::rendering::SET_MAIN_CAMERA) { entity: " + std::to_string(setCameraParams.cameraEntity) + " }");

        m_mainCamera = setCameraParams.cameraEntity;

        // we'll have camera match to whatever the current viewport size is
        // dynamo must be non-null when calling here
        int viewportSize[4];
        m_attachedRenderDynamo->read_viewport_size(viewportSize);
        _resize_main_camera(viewportSize[2], viewportSize[3]);
    }
    
    void RenderSynchro::_resize_handler(EventMessage resizeEvent) 
    {
        events::window::RESIZE_params resizeParams;
        resizeEvent >> resizeParams;
        
        PLEEPLOG_TRACE("Handling event " + std::to_string(events::window::RESIZE) + " (events::window::RESIZE) { width: " + std::to_string(resizeParams.width) + ", height: " + std::to_string(resizeParams.height) + " }");

        // we'll have the camera always match to window resolution
        _resize_main_camera(resizeParams.width, resizeParams.height);
    }
    
    void RenderSynchro::_resize_main_camera(int width, int height) 
    {
        PLEEPLOG_TRACE("Overwriting registered camera with dimensions (" + std::to_string(width) + ", " + std::to_string(height) + ")");

        // camera components must be registered and this entity must have a camera component
        CameraComponent& mainCamInfo = m_ownerCosmos->get_component<CameraComponent>(m_mainCamera);
        mainCamInfo.viewWidth = width;
        mainCamInfo.viewHeight = height;

        // on next camera submit these dimensions will be checked and render relays will resize themselves accordingly
    }
}