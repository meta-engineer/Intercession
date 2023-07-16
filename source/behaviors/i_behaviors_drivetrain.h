#ifndef I_BEHAVIORS_DRIVETRAIN_H
#define I_BEHAVIORS_DRIVETRAIN_H

//#include "intercession_pch.h"
#define GLM_FORCE_SILENT_WARNINGS
#include <glm/glm.hpp>
#include <glm/gtx/intersect.hpp>
#include <memory>

#include "physics/collider_packet.h"
#include "behaviors/behaviors_library.h"

namespace pleep
{
    // Forward declare BehaviorsComponent so it can pass itself to drivetrain callbacks
    struct BehaviorsComponent;

    // Contains all virtual methods that behaviors users can call
    // I guess behaviors users will just... implicitly know which methods they should call
    // EX: on_fixed_update, on_frame_update, on_collision, etc...
    // BehaviorsDrivetrain subclasses should only store members for globally applicable data
    // any entity specific data should be contained in behaviors component or a standalone component
    class I_BehaviorsDrivetrain
    {
    protected:
        I_BehaviorsDrivetrain() = default;
    public:
        virtual ~I_BehaviorsDrivetrain() = default;

        // invoked once per fixed interval per entity which holds it in their BehaviorsComponent
        // Passes individual BehaviorsPacket members to avoid circular dependency
        virtual void on_fixed_update(double deltaTime, BehaviorsComponent& behaviors, Entity entity, std::weak_ptr<Cosmos> owner);
        
        // invoked once per frame interval per entity which holds it in their BehaviorsComponent
        // Passes individual BehaviorsPacket members to avoid circular dependency
        virtual void on_frame_update(double deltaTime, BehaviorsComponent& behaviors, Entity entity, std::weak_ptr<Cosmos> owner);

        // invoked when the behaviors owner collides with another collider
        // we'll copy parameters just incase
        // collision metadata is relative to collidee
        // NOTE: both colliders will have their on_collision behaviors invoked independantly
        virtual void on_collision(ColliderPacket colliderData, ColliderPacket collideeData, glm::vec3 collisionNormal, float collisionDepth, glm::vec3 collisionPoint);

        // TODO: how do we detect/track the "start" and "end" of a collision? should collision relay dispatch?
        virtual void on_collision_enter();
        virtual void on_collision_exit();
        
        // Stores the type of behaviors loaded here from library for serialization
        // this matches the pattern of supermesh's m_sourceFilename.
        // TODO this is public (for serializer access) and perhaps unsafe
        //     maybe make this protected and add operators/library as friends?
        BehaviorsLibrary::BehaviorsType m_libraryType = BehaviorsLibrary::BehaviorsType::none;
    };
}

#endif // I_BEHAVIORS_DRIVETRAIN_H