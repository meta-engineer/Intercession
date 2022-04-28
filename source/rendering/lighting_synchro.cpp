#include "lighting_synchro.h"

#include <exception>
#include "logging/pleep_log.h"
#include "core/cosmos.h"

#include "physics/transform_component.h"
#include "rendering/light_source_component.h"
#include "rendering/light_source_packet.h"

namespace pleep
{
    void LightingSynchro::update() 
    {
        // No owner is a fatal error
        if (m_ownerCosmos == nullptr)
        {
            PLEEPLOG_ERROR("Lighting Synchro has no owner Cosmos");
            throw std::runtime_error("Lighting Synchro started update without owner Cosmos");
        }

        // no dynamo is a mistake, not necessarily an error
        if (m_attachedRenderDynamo == nullptr)
        {
            PLEEPLOG_WARN("Lighting Synchro update was called without an attached Dynamo");
            return;
        }

        // feed components of m_entities to attached RenderDynamo
        for (Entity const& entity : m_entities)
        {
            TransformComponent& transform = m_ownerCosmos->get_component<TransformComponent>(entity);
            LightSourceComponent& light = m_ownerCosmos->get_component<LightSourceComponent>(entity);

            m_attachedRenderDynamo->submit(LightSourcePacket{ transform, light });
        }

        // this is pre-processing and SHOULD be run before RenderSynchro's update
        // this means light sources are used in an "immediate mode" and are
        //   re-submitted every frame. Assuming light sources would frequently
        //   update anyway, this isn't THAT much overhead
    }
    
    void LightingSynchro::attach_dynamo(RenderDynamo* contextDynamo) 
    {
        m_attachedRenderDynamo = contextDynamo;
    }
}