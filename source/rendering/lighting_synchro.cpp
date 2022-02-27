#include "lighting_synchro.h"

#include <exception>
#include "logging/pleep_log.h"
#include "core/cosmos.h"

#include "physics/transform_component.h"
#include "rendering/light_source_component.h"
#include "rendering/light_source_packet.h"

namespace pleep
{
    void LightingSynchro::update(double deltaTime) 
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

        // feed components of m_entities to attached RenderDynamo
        for (Entity const& entity : m_entities)
        {
            TransformComponent& transform = m_ownerCosmos->get_component<TransformComponent>(entity);
            LightSourceComponent& light = m_ownerCosmos->get_component<LightSourceComponent>(entity);

            m_attachedRenderDynamo->submit(LightSourcePacket{ transform, light });
        }

        // this is pre-processing and needs to be run before RenderSynchro's update
        // it will then call the final run_relays() on dynamo
        // this means light sources are used in an "immediate mode" and are
        //   re-submitted every frame. Assuming light sources would frequently
        //   update anyway, this isn't THAT much overhead
        UNREFERENCED_PARAMETER(deltaTime);
    }
    
    void LightingSynchro::attach_dynamo(RenderDynamo* contextDynamo) 
    {
        m_attachedRenderDynamo = contextDynamo;
    }
}