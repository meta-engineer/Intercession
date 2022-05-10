#ifndef I_PHYSICS_RESPONSE_COMPONENT_H
#define I_PHYSICS_RESPONSE_COMPONENT_H

//#include "intercession_pch.h"
#define GLM_FORCE_SILENT_WARNINGS
#include <glm/glm.hpp>
#include <glm/gtx/intersect.hpp>

#include "logging/pleep_log.h"
#include "physics/collider_packet.h"

namespace pleep
{
    // Forward declare all class types to dispatch to
    struct RigidBodyComponent;
    struct SpringBodyComponent;

    struct IPhysicsResponseComponent
    {
        // ***** Universal response attributes *****
        // Entity may want unlocked orientation (for kinematic motion)
        // but not want physics responses to cause orientation changes
        bool m_influenceOrientation = true;

        // double dispatch for each response subclass
        virtual void collision_response(IPhysicsResponseComponent* otherPhysicsResponse, ColliderPacket& thisData, ColliderPacket& otherData, glm::vec3& collisionNormal, float& collisionDepth, glm::vec3& collisionPoint) = 0;
        
        // implement subclass's response with Rigid Body
        virtual void collision_response(RigidBodyComponent* otherPhysicsResponse, ColliderPacket& thisData, ColliderPacket& otherData, glm::vec3& collisionNormal, float& collisionDepth, glm::vec3& collisionPoint)
        {
            PLEEPLOG_WARN("No implementation for response between this type (?) and RigidBodyComponent");
            UNREFERENCED_PARAMETER(otherPhysicsResponse);
            UNREFERENCED_PARAMETER(thisData);
            UNREFERENCED_PARAMETER(otherData);
            UNREFERENCED_PARAMETER(collisionNormal);
            UNREFERENCED_PARAMETER(collisionDepth);
            UNREFERENCED_PARAMETER(collisionPoint);
        }
        
        // implement subclass's response with Spring Body
        virtual void collision_response(SpringBodyComponent* otherPhysicsResponse, ColliderPacket& thisData, ColliderPacket& otherData, glm::vec3& collisionNormal, float& collisionDepth, glm::vec3& collisionPoint)
        {
            PLEEPLOG_WARN("No implementation for response between this type (?) and SpringBodyComponent");
            UNREFERENCED_PARAMETER(otherPhysicsResponse);
            UNREFERENCED_PARAMETER(thisData);
            UNREFERENCED_PARAMETER(otherData);
            UNREFERENCED_PARAMETER(collisionNormal);
            UNREFERENCED_PARAMETER(collisionDepth);
            UNREFERENCED_PARAMETER(collisionPoint);
        }

        // potentially useful asymptotic damping factor?
        static float calculate_asymp_damping(glm::vec3 vector, float invFactor)
        {
            return -1.0f / (1.0f +
                (vector.x*vector.x 
                + vector.y*vector.y
                + vector.z*vector.z) * invFactor) + 1.0f;
        }
    };
}

#endif // I_PHYSICS_RESPONSE_COMPONENT_H