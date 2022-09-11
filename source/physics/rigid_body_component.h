#ifndef RIGID_BODY_COMPONENT_H
#define RIGID_BODY_COMPONENT_H

//#include "intercession_pch.h"
#include "physics/i_physics_response_component.h"
#include "events/message.h"
#include "logging/pleep_log.h"

namespace pleep
{
    // define attributes needed for rigidbody physics response
    struct RigidBodyComponent : public I_PhysicsResponseComponent
    {
        // !!! Make sure to update serializers at page end if members are updated !!!
        static const uint32_t dataSize = sizeof(float) * 3 + sizeof(bool);

        // proportion of energy *retained* normal response
        float restitution       = 0.70f;
        // proportion of energy *lost* through friction
        float staticFriction    = 0.45f;
        float dynamicFriction   = 0.45f;
        
        // Entity may want unlocked orientation (for kinematic motion)
        // but not want physics responses to cause orientation changes
        bool influenceOrientation = true;

        // ***** Collision Response methods *****
        // double dispatch other
        virtual void collision_response(I_PhysicsResponseComponent* otherBody, ColliderPacket& thisData, ColliderPacket& otherData, glm::vec3& collisionNormal, float& collisionDepth, glm::vec3& collisionPoint) override;
        
        // rigid-rigid response
        virtual void collision_response(RigidBodyComponent* otherRigidBody, ColliderPacket& thisData, ColliderPacket& otherData, glm::vec3& collisionNormal, float& collisionDepth, glm::vec3& collisionPoint) override;
        
        // rigid-spring response
        virtual void collision_response(SpringBodyComponent* otherSpringBody, ColliderPacket& thisData, ColliderPacket& otherData, glm::vec3& collisionNormal, float& collisionDepth, glm::vec3& collisionPoint) override;
    };
    
    // Virtual dispatch makes RigidBodyComponent non-POD, so we must override Message serialization
    template<typename T_Msg>
    Message<T_Msg>& operator<<(Message<T_Msg>& msg, const RigidBodyComponent& data)
    {
        // make sure stream operators are updated if members are updated
        static_assert(RigidBodyComponent::dataSize == 13, "RigidBodyComponent Message serializer found unexpected data size");
        
        uint32_t i = msg.size();
        // resize all at once
        msg.body.resize(msg.body.size() + RigidBodyComponent::dataSize);

        std::memcpy(msg.body.data() + i, &(data.restitution), sizeof(float));
        i += sizeof(float);
        
        std::memcpy(msg.body.data() + i, &(data.staticFriction), sizeof(float));
        i += sizeof(float);
        
        std::memcpy(msg.body.data() + i, &(data.dynamicFriction), sizeof(float));
        i += sizeof(float);
        
        std::memcpy(msg.body.data() + i, &(data.influenceOrientation), sizeof(bool));
        i += sizeof(bool);

        // recalc message size
        msg.header.size = msg.size();

        return msg;
    }
    template<typename T_Msg>
    Message<T_Msg>& operator>>(Message<T_Msg>& msg, RigidBodyComponent& data)
    {
        // stream out when no data is available;
        assert(msg.size() >= RigidBodyComponent::dataSize);
        
        // track index at the start of the data on "top" of the stack
        uint32_t i = msg.size() - RigidBodyComponent::dataSize;

        std::memcpy(&(data.restitution), msg.body.data() + i, sizeof(float));
        i += sizeof(float);
        
        std::memcpy(&(data.staticFriction), msg.body.data() + i, sizeof(float));
        i += sizeof(float);
        
        std::memcpy(&(data.dynamicFriction), msg.body.data() + i, sizeof(float));
        i += sizeof(float);
        
        std::memcpy(&(data.influenceOrientation), msg.body.data() + i, sizeof(bool));
        i += sizeof(bool);
        
        // shrink, removing end of stack (constant time)
        msg.body.resize(msg.size() - RigidBodyComponent::dataSize);

        // recalc message size
        msg.header.size = msg.size();
        
        return msg;
    }

    // Testing
    inline bool operator==(const RigidBodyComponent& lhs, const RigidBodyComponent& rhs)
    {
        return lhs.restitution == rhs.restitution
            && lhs.staticFriction == rhs.staticFriction
            && lhs.dynamicFriction == rhs.dynamicFriction
            && lhs.influenceOrientation == rhs.influenceOrientation;
    }
}

#endif // RIGID_BODY_COMPONENT_H