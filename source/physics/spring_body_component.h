#ifndef SPRING_BODY_COMPONENT_H
#define SPRING_BODY_COMPONENT_H

//#include "intercession_pch.h"
#include "physics/i_physics_response_component.h"

namespace pleep
{
    struct SpringBodyComponent : public IPhysicsResponseComponent
    {
        // maximum length of the spring is defined by the entity's associated collider
        float restLength = 0.9f;
        float stiffness  = 1.0f;
        float damping    = 0.1f;
        // proportion of energy *lost* through friction
        float staticFriction    = 0.00f;
        float dynamicFriction   = 0.00f;

        // ***** Collision Response methods *****
        // double dispatch other
        void collision_response(IPhysicsResponseComponent* otherBody, ColliderPacket& thisData, ColliderPacket& otherData, glm::vec3& collisionNormal, float& collisionDepth, glm::vec3& collisionPoint) override;

        // spring-spring response
        void collision_response(SpringBodyComponent* otherSpringBody, ColliderPacket& thisData, ColliderPacket& otherData, glm::vec3& collisionNormal, float& collisionDepth, glm::vec3& collisionPoint) override;
        
        // spring-rigid response
        void collision_response(RigidBodyComponent* otherRigidBody, ColliderPacket& thisData, ColliderPacket& otherData, glm::vec3& collisionNormal, float& collisionDepth, glm::vec3& collisionPoint) override;
    };
}

#endif // SPRING_BODY_COMPONENT_H