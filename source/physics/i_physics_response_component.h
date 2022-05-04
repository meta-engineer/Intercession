#ifndef I_PHYSICS_RESPONSE_COMPONENT_H
#define I_PHYSICS_RESPONSE_COMPONENT_H

//#include "intercession_pch.h"
#include <glm/glm.hpp>
#include <glm/gtx/intersect.hpp>

#include "logging/pleep_log.h"
#include "physics/collider_packet.h"

namespace pleep
{
    // Forward declare all class types to dispatch to
    struct RigidBodyComponent;

    struct IPhysicsResponseComponent
    {
        // double dispatch for each response subclass
        virtual void collision_response(IPhysicsResponseComponent* otherPhysicsResponse, ColliderPacket& thisData, ColliderPacket& otherData, glm::vec3& collisionNormal, float& collisionDepth, glm::vec3& collisionPoint) = 0;
        
        virtual void collision_response(RigidBodyComponent* otherPhysicsResponse, ColliderPacket& thisData, ColliderPacket& otherData, glm::vec3& collisionNormal, float& collisionDepth, glm::vec3& collisionPoint)
        {
            PLEEPLOG_WARN("No implementation for collision between this type (?) and BoxColliderComponent");
            UNREFERENCED_PARAMETER(otherPhysicsResponse);
            UNREFERENCED_PARAMETER(thisData);
            UNREFERENCED_PARAMETER(otherData);
            UNREFERENCED_PARAMETER(collisionNormal);
            UNREFERENCED_PARAMETER(collisionDepth);
            UNREFERENCED_PARAMETER(collisionPoint);
        }

        // potentially useful exponential damping factor?
        static float calculate_damping(glm::vec3 vector, float invFactor)
        {
            return -1.0f / (1.0f +
                (vector.x*vector.x 
                + vector.y*vector.y
                + vector.z*vector.z) * invFactor) + 1.0f;
        }
    };
}

#endif // I_PHYSICS_RESPONSE_COMPONENT_H