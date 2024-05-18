#include "animation_synchro.h"

#include <exception>

#include "logging/pleep_log.h"
#include "core/cosmos.h"

#include "rendering/renderable_component.h"
#include "rendering/animation_component.h"
#include "rendering/animation_packet.h"

namespace pleep
{
    AnimationSynchro::~AnimationSynchro()
    {
        // clear attached dynamo & handlers
        this->attach_dynamo(nullptr);
    }

    void AnimationSynchro::update()
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
            throw std::runtime_error("AnimationSynchro started update without owner Cosmos");
        }
        
        // feed components of m_entities to attached RenderDynamo
        // I should implicitly know my signature and therefore what components i can fetch
        for (Entity const& entity : m_entities)
        {
            RenderableComponent& renderable = cosmos->get_component<RenderableComponent>(entity);
            AnimationComponent& animatable = cosmos->get_component<AnimationComponent>(entity);

            m_attachedRenderDynamo->submit(AnimationPacket{ renderable, animatable });
        }
    }

    Signature AnimationSynchro::derive_signature()
    {
        std::shared_ptr<Cosmos> cosmos = m_ownerCosmos.lock();
        // No owner is a fatal error
        if (m_ownerCosmos.expired())
        {
            PLEEPLOG_ERROR("Cannot derive signature for null Cosmos");
            throw std::runtime_error("AnimationSynchro started signature derivation without owner Cosmos");
        }

        Signature sign;

        try
        {
            sign.set(cosmos->get_component_type<RenderableComponent>());
            sign.set(cosmos->get_component_type<AnimationComponent>());
        }
        catch(const std::exception& e)
        {
            // Component Registry already logs error
            UNREFERENCED_PARAMETER(e);
            sign.reset();
            PLEEPLOG_ERROR("Synchro could not get desired component types from cosmos. Have RenderableComponent and AnimationComponent been registered?");
        }

        return sign;
    }

    void AnimationSynchro::attach_dynamo(std::shared_ptr<RenderDynamo> contextDynamo)
    {
        // We cannot subscribe to events until the dynamo is attached (to have broker access)
        // So we have to do it here, but make sure we don't double subscribe if dynamo changes

        // ...

        m_attachedRenderDynamo = contextDynamo;

        // ...
    }
}