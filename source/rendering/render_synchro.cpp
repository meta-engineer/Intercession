#include "render_synchro.h"

#include <exception>

#include "logging/pleep_log.h"
#include "core/cosmos.h"

#include "physics/transform_component.h"
#include "rendering/camera_component.h"
#include "rendering/model_component.h"
#include "rendering/render_packet.h"

namespace pleep
{
    void RenderSynchro::update(double deltaTime)
    {
        // No owner is a fatal error
        if (m_ownerCosmos == nullptr)
        {
            PLEEPLOG_ERROR("Render Synchro does not owner Cosmos");
            throw std::runtime_error("Render Synchro started update without owner Cosmos");
        }

        // no dynamo is a mistake, not necessarily an error
        if (m_attachedRenderDynamo == nullptr)
        {
            PLEEPLOG_WARN("Render Synchro update was called without an attached Dynamo");
            return;
        }

        // update view info with registered camera
        // re-get camera each time becuase ECS pointers are volatile
        CameraComponent& viewCamera = m_ownerCosmos->get_component<CameraComponent>(m_mainCamera);
        TransformComponent& viewTransform = m_ownerCosmos->get_component<TransformComponent>(m_mainCamera);
        m_attachedRenderDynamo->set_world_to_view(get_lookAt(viewTransform, viewCamera));
        m_attachedRenderDynamo->set_projection(get_projection(viewCamera));
        m_attachedRenderDynamo->set_viewPos(viewTransform.origin);

        // feed components of m_entities to attached ControlDynamo
        // I should implicitly know my signature and therefore what components i can fetch
        for (Entity const& entity : m_entities)
        {
            TransformComponent& transform = m_ownerCosmos->get_component<TransformComponent>(entity);
            ModelComponent& model = m_ownerCosmos->get_component<ModelComponent>(entity);
            
            // NOTE: eventually ModelComponent will directly own mesh vector, not just wrap old/model
            // we'll pass each mesh to renderer, NOT a model
            // Are meshes independant?
            //   any other data dynamo could need from entity?
            //   are there model-wide options for rendering? outlines? palettes?
            for (Mesh& mesh : model.model->meshes)
            {
                m_attachedRenderDynamo->submit(RenderPacket{ transform, mesh });
            }
        }

        // once command queue is implemented this will flush them through relays
        m_attachedRenderDynamo->run_relays(deltaTime);

        // Do not flush Dynamo yet as other components (Context) may draw further
        // and then Context will flush at frame end
    }
    
    void RenderSynchro::attach_dynamo(RenderDynamo* contextDynamo) 
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
    
    void RenderSynchro::_set_main_camera_handler(Event setCameraEvent) 
    {
        events::rendering::set_main_camera::Params cameraParams = setCameraEvent.get_param<events::rendering::set_main_camera::Params>();
        
        PLEEPLOG_TRACE("Handling event " + std::to_string(events::rendering::SET_MAIN_CAMERA) + " (events::rendering::SET_MAIN_CAMERA) { entity: " + std::to_string(cameraParams.cameraEntity) + " }");

        m_mainCamera = cameraParams.cameraEntity;

        // dynamo must be non-null when calling here
        int viewportSize[4];
        m_attachedRenderDynamo->read_viewport_size(viewportSize);
        _resize_main_camera(viewportSize[2], viewportSize[3]);
    }
    
    void RenderSynchro::_resize_handler(Event resizeEvent) 
    {
        events::window::resize::Params resizeParams = resizeEvent.get_param<events::window::resize::Params>();
        
        PLEEPLOG_TRACE("Handling event " + std::to_string(events::window::RESIZE) + " (events::window::RESIZE) { width: " + std::to_string(resizeParams.width) + ", height: " + std::to_string(resizeParams.height) + " }");

        // for now always match camera to window resolution
        _resize_main_camera(resizeParams.width, resizeParams.height);
    }
    
    void RenderSynchro::_resize_main_camera(int width, int height) 
    {
        PLEEPLOG_TRACE("Overwriting registered camera with viewport dimensions: " + std::to_string(width) + ", " + std::to_string(height));

        // camera components must be registered and this entity must have a camera component
        CameraComponent& mainCamInfo = m_ownerCosmos->get_component<CameraComponent>(m_mainCamera);
        mainCamInfo.viewWidth = width;
        mainCamInfo.viewHeight = height;

        m_attachedRenderDynamo->resize_framebuffers(mainCamInfo.viewWidth, mainCamInfo.viewHeight);
    }
}