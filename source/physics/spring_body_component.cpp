#include "spring_body_component.h"
#include "core/cosmos.h"
#include "physics/physics_component.h"
#include "physics/rigid_body_component.h"

namespace pleep
{
    void SpringBodyComponent::collision_response(IPhysicsResponseComponent* otherBody, ColliderPacket& thisData, ColliderPacket& otherData, glm::vec3& collisionNormal, float& collisionDepth, glm::vec3& collisionPoint)
    {
            // switch collisionNormal & collisionPoint
            glm::vec3 invCollisionNormal = -collisionNormal;
            glm::vec3 invCollisionPoint = collisionPoint - (collisionNormal * collisionDepth);
            otherBody->collision_response(this, otherData, thisData, invCollisionNormal, collisionDepth, invCollisionPoint);
    }

    void SpringBodyComponent::collision_response(SpringBodyComponent* otherSpringBody, ColliderPacket& thisData, ColliderPacket& otherData, glm::vec3& collisionNormal, float& collisionDepth, glm::vec3& collisionPoint)
    {
        // find spring force of each body, combine, then apply equal-and-opposite
        // apply friction

        PLEEPLOG_WARN("Spring-Spring response not implemented yet.");
        UNREFERENCED_PARAMETER(otherSpringBody);
        UNREFERENCED_PARAMETER(thisData);
        UNREFERENCED_PARAMETER(otherData);
        UNREFERENCED_PARAMETER(collisionNormal);
        UNREFERENCED_PARAMETER(collisionDepth);
        UNREFERENCED_PARAMETER(collisionPoint);
    }
    
    void SpringBodyComponent::collision_response(RigidBodyComponent* otherRigidBody, ColliderPacket& thisData, ColliderPacket& otherData, glm::vec3& collisionNormal, float& collisionDepth, glm::vec3& collisionPoint)
    {
        // find spring force of myself, then apply equal-and-opposite
        // apply friction

        PLEEPLOG_WARN("Spring-Rigid response not implemented yet.");
        UNREFERENCED_PARAMETER(otherRigidBody);
        UNREFERENCED_PARAMETER(thisData);
        UNREFERENCED_PARAMETER(otherData);
        UNREFERENCED_PARAMETER(collisionNormal);
        UNREFERENCED_PARAMETER(collisionDepth);
        UNREFERENCED_PARAMETER(collisionPoint);
    }
}