#include "render_synchro.h"

#include <exception>

#include "logging/pleep_log.h"
#include "physics/transform_component.h"
#include "rendering/model_component.h"

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
        

        // any dynamo specific initialization (glClear)
        m_attachedRenderDynamo->prime();

        // feed components of m_entities to attached ControlDynamo
        // I should implicitly know my signature and therefore what components i can fetch
        for (Entity const& entity : m_entities)
        {
            TransformComponent transform = m_ownerCosmos->get_component<TransformComponent>(entity);
            ModelComponent model = m_ownerCosmos->get_component<ModelComponent>(entity);
            
            //m_attachedRenderDynamo->submit();
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

            m_attachedRenderDynamo->get_shared_broker()->remove_listener(METHOD_LISTENER(events::rendering::SET_MAIN_CAMERA, RenderSynchro::_register_main_camera));
        }

        m_attachedRenderDynamo = contextDynamo;

        if (m_attachedRenderDynamo)
        {
            // remember this is broadcast so if there are more than 1 synchro they will all be set
            contextDynamo->get_shared_broker()->add_listener(METHOD_LISTENER(events::rendering::SET_MAIN_CAMERA, RenderSynchro::_register_main_camera));
        }
    }
    
    void RenderSynchro::_register_main_camera(Event setCameraEvent) 
    {
        events::rendering::set_main_camera::Params cameraParams = setCameraEvent.get_param<events::rendering::set_main_camera::Params>();
        
        PLEEPLOG_TRACE("Handling event " + std::to_string(events::rendering::SET_MAIN_CAMERA) + " (events::rendering::SET_MAIN_CAMERA) { entity: " + std::to_string(cameraParams.cameraEntity) + " }");

        m_mainCamera = cameraParams.cameraEntity;
    }
}