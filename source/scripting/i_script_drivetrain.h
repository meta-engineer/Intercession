#ifndef I_SCRIPT_DRIVETRAIN_H
#define I_SCRIPT_DRIVETRAIN_H

//#include "intercession_pch.h"
#define GLM_FORCE_SILENT_WARNINGS
#include <glm/glm.hpp>
#include <glm/gtx/intersect.hpp>
#include <memory>

#include "physics/collider_packet.h"
#include "scripting/script_library.h"

namespace pleep
{
    // Forward declare ScriptComponent so it can pass itself to drivetrain callbacks
    struct ScriptComponent;

    // Contains all virtual methods that script users can call
    // I guess script users will just... implicitly know which methods they should call
    // EX: on_fixed_update, on_frame_update, on_collision, etc...
    // ScriptDrivetrain subclasses should only store members for globally applicable data
    // any entity specific data should be contained in script component or a standalone component
    class I_ScriptDrivetrain
    {
    protected:
        I_ScriptDrivetrain() = default;
    public:
        virtual ~I_ScriptDrivetrain() = default;

        // invoked once per fixed interval per entity which holds it in their ScriptComponent
        virtual void on_fixed_update(double deltaTime, ScriptComponent& script, Entity entity = NULL_ENTITY, std::shared_ptr<Cosmos> owner = nullptr);
        
        // invoked once per frame interval per entity which holds it in their ScriptComponent
        virtual void on_frame_update(double deltaTime, ScriptComponent& script, Entity entity = NULL_ENTITY, std::shared_ptr<Cosmos> owner = nullptr);

        // invoked at most once per fixed interval per entity which holds it as their
        //   I_ColliderComponent::scriptTarget when it collides with another collider
        // we'll copy parameters just incase
        // collision metadata is relative to collidee
        // NOTE: both colliders will have their on_collision scripts invoked independantly
        virtual void on_collision(ColliderPacket colliderData, ColliderPacket collideeData, glm::vec3 collisionNormal, float collisionDepth, glm::vec3 collisionPoint);

        // TODO: how do we detect/track the "start" and "end" of a collision? should collision relay dispatch?
        virtual void on_collision_enter();
        virtual void on_collision_exit();
        
        // Stores the type of script loaded here from library for serialization
        // this matches the pattern of supermesh's m_sourceFilename.
        // TODO this is public (for serializer access) and perhaps unsafe
        //     maybe make this protected and add operators/library as friends?
        ScriptLibrary::ScriptType m_libraryType = ScriptLibrary::ScriptType::none;
    };
}

#endif // I_SCRIPT_DRIVETRAIN_H