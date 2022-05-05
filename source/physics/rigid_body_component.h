#ifndef RIGID_BODY_COMPONENT_H
#define RIGID_BODY_COMPONENT_H

//#include "intercession_pch.h"
#include "physics/i_physics_response_component.h"

namespace pleep
{
    // define attributes needed for rigidbody physics response
    struct RigidBodyComponent : public IPhysicsResponseComponent
    {
        // proportion of energy *retained* normal response
        float restitution       = 0.70f;
        // proportion of energy *lost* through friction
        float staticFriction    = 0.45f;
        float dynamicFriction   = 0.45f;

        // ***** Collision Response methods *****
        // double dispatch other
        void collision_response(IPhysicsResponseComponent* otherBody, ColliderPacket& thisData, ColliderPacket& otherData, glm::vec3& collisionNormal, float& collisionDepth, glm::vec3& collisionPoint) override;
        
        // rigid-rigid response
        void collision_response(RigidBodyComponent* otherRigidBody, ColliderPacket& thisData, ColliderPacket& otherData, glm::vec3& collisionNormal, float& collisionDepth, glm::vec3& collisionPoint) override;
        
        // rigid-spring response
        void collision_response(SpringBodyComponent* otherSpringBody, ColliderPacket& thisData, ColliderPacket& otherData, glm::vec3& collisionNormal, float& collisionDepth, glm::vec3& collisionPoint) override;
    };
}

#endif // RIGID_BODY_COMPONENT_H