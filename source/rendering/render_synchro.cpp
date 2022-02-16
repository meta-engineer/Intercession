#include "render_synchro.h"

#include <exception>

#include "logging/pleep_log.h"
#include "physics/transform_component.h"
#include "rendering/model_component.h"
#include "rendering/camera_component.h"

namespace pleep
{
    RenderSynchro::RenderSynchro(Cosmos* owner) 
        : m_ownerCosmos(owner)
        , m_attachedRenderDynamo(nullptr)
    {

    }
    
    RenderSynchro::~RenderSynchro() 
    {
        // I do not own my Cosmos (parent) or Dynamo
    }
    
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

        // feed components of m_entities to attached ControlDynamo
        // I should implicitly know my signature and therefore what components i can fetch
        for (Entity const& entity : m_entities)
        {
            TransformComponent transform = m_ownerCosmos->get_component<TransformComponent>(entity);
            ModelComponent model = m_ownerCosmos->get_component<ModelComponent>(entity);
            
            //m_attachedRenderDynamo->submit(transform, model->meshes[i]);
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

        _resize_main_camera(resizeParams.width, resizeParams.height);
    }
    
    void RenderSynchro::_resize_main_camera(int width, int height) 
    {
        PLEEPLOG_TRACE("Overwriting registered camera with viewport dimensions: " + std::to_string(width) + ", " + std::to_string(height));

        // camera components must be registered and this entity must have a camera component
        CameraComponent mainCamInfo = m_ownerCosmos->get_component<CameraComponent>(m_mainCamera);
        mainCamInfo.viewWidth = width;
        mainCamInfo.viewHeight = height;
    }
}