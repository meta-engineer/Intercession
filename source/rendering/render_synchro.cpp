#include "render_synchro.h"

#include <exception>

#include "logging/pleep_log.h"
#include "core/cosmos.h"

#include "physics/transform_component.h"
#include "rendering/camera_component.h"
#include "rendering/renderable_component.h"
#include "rendering/render_packet.h"
#include "rendering/debug_render_packet.h"
#include "physics/box_collider_component.h"
#include "physics/ray_collider_component.h"

namespace pleep
{
    RenderSynchro::~RenderSynchro()
    {
        // clear attached dynamo & handlers
        this->attach_dynamo(nullptr);
    }

    void RenderSynchro::update()
    {
        // servers will not have render dynamos
        if (m_attachedRenderDynamo == nullptr)
        {
            //PLEEPLOG_WARN("Synchro update was called without an attached Dynamo");
            return;
        }

        std::shared_ptr<Cosmos> cosmos = m_ownerCosmos.lock();
        // No owner is a fatal error
        if (m_ownerCosmos.expired())
        {
            PLEEPLOG_ERROR("Synchro has no owner Cosmos");
            throw std::runtime_error("RenderSynchro started update without owner Cosmos");
        }

        // update view info with registered camera
        // re-get camera each time because ECS pointers are volatile
        if (cosmos->entity_exists(m_mainCamera))
        {
            m_attachedRenderDynamo->submit(CameraPacket {
                cosmos->get_component<TransformComponent>(m_mainCamera),
                cosmos->get_component<CameraComponent>(m_mainCamera)
            });
        }
        else
        {
            m_mainCamera = NULL_ENTITY;
            //PLEEPLOG_WARN("Update called while no main camera entity was set");
        }

        // feed components of m_entities to attached RenderDynamo
        // I should implicitly know my signature and therefore what components i can fetch
        for (Entity const& entity : m_entities)
        {
            TransformComponent& transform = cosmos->get_component<TransformComponent>(entity);
            RenderableComponent& renderable = cosmos->get_component<RenderableComponent>(entity);
            
            // catch null supermesh and don't even bother dynamo with it
            // except if render colliders is true
            //if (renderable.meshData == nullptr) continue;

            // Update renderable members based on other components...

            // DEBUG: set highlight based on timestream state
            if (cosmos->get_timestream_state(entity).first == TimestreamState::superposition)
            {
                renderable.highlight = true;
            }
            else
            {
                renderable.highlight = false;
            }

            m_attachedRenderDynamo->submit(RenderPacket{ transform, renderable });

            // DEBUG: Look for collider components for debug rendering?
            // we only need base transform, collider transform, BasicSupermeshType, and maybe Entity for colour seed?
            if (cosmos->has_component<BoxColliderComponent>(entity) &&
                cosmos->get_component<BoxColliderComponent>(entity).isActive == true)
            {
                m_attachedRenderDynamo->submit(DebugRenderPacket{
                    entity,
                    cosmos->get_component<BoxColliderComponent>(entity).compose_transform(transform),
                    ModelManager::BasicSupermeshType::cube
                });
            }
            if (cosmos->has_component<RayColliderComponent>(entity) &&
                cosmos->get_component<RayColliderComponent>(entity).isActive == true)
            {
                m_attachedRenderDynamo->submit(DebugRenderPacket{
                    entity,
                    cosmos->get_component<RayColliderComponent>(entity).compose_transform(transform),
                    ModelManager::BasicSupermeshType::vector
                });
            }
        }

        // CosmosContext will flush dynamo relays once all synchros are done
        // Other components (Context) may draw further and then Context will flush at frame end
    }
    
    Signature RenderSynchro::derive_signature() 
    {
        std::shared_ptr<Cosmos> cosmos = m_ownerCosmos.lock();
        // No owner is a fatal error
        if (m_ownerCosmos.expired())
        {
            PLEEPLOG_ERROR("Cannot derive signature for null Cosmos");
            throw std::runtime_error("RenderSynchro started signature derivation without owner Cosmos");
        }

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

        std::shared_ptr<Cosmos> cosmos = m_ownerCosmos.lock();

        // No owner means we can't verify that this will be a valid camera at update time
        if (cosmos == nullptr || m_ownerCosmos.expired())
        {
            PLEEPLOG_ERROR("Cannot validate camera entity without an owner Cosmos");
            throw std::runtime_error("RenderSynchro handled SET_MAIN_CAMERA without owner Cosmos");
        }

        // ensure entity has camera component AT LEAST at time of setting
        try
        {
            cosmos->get_component<CameraComponent>(setCameraParams.cameraEntity);
        }
        catch(const std::exception& e)
        {
            UNREFERENCED_PARAMETER(e);
            PLEEPLOG_WARN("Tried to set main camera as entity " + std::to_string(setCameraParams.cameraEntity) + " which has no CameraComponent, ignoring...");
            return;
        }

        // camera is good
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
        if (m_mainCamera == NULL_ENTITY) return;

        PLEEPLOG_TRACE("Overwriting registered camera with dimensions (" + std::to_string(width) + ", " + std::to_string(height) + ")");

        std::shared_ptr<Cosmos> cosmos = m_ownerCosmos.lock();

        // No owner means we can't verify that this will be a valid camera at update time
        if (cosmos == nullptr || m_ownerCosmos.expired())
        {
            PLEEPLOG_ERROR("Cannot write to camera entity without an owner Cosmos");
            throw std::runtime_error("RenderSynchro handled RESIZE_MAIN_CAMERA without owner Cosmos");
        }

        // camera components must be registered and this entity must have a camera component
        try
        {
            CameraComponent& mainCamInfo = cosmos->get_component<CameraComponent>(m_mainCamera);
            mainCamInfo.viewWidth = width;
            mainCamInfo.viewHeight = height;
        }
        catch(const std::exception& e)
        {
            UNREFERENCED_PARAMETER(e);
            PLEEPLOG_WARN("Tried to set dimensions of main camera entity which has no CameraComponent, ignoring...");
            return;
        }

        // on next camera submit these dimensions will be checked and render relays will resize themselves accordingly
    }
}