#ifndef SPRING_BODY_COMPONENT_H
#define SPRING_BODY_COMPONENT_H

//#include "intercession_pch.h"
#include "physics/i_physics_response_component.h"
#include "events/message.h"
#include "logging/pleep_log.h"

namespace pleep
{
    struct SpringBodyComponent : public I_PhysicsResponseComponent
    {
        // !!! Make sure to update serializers at page end if members are updated !!!
        static const uint32_t dataSize = sizeof(float) * 5 
            + sizeof(bool);

        // maximum length of the spring is defined by the entity's associated collider
        // rest length is distance from surface along penetration normal
        // any distance > 0 will have a "pulling" force
        float restLength = 0.1f;
        float stiffness  = 100.0f;
        float damping    = 10.0f;
        // proportion of energy *lost* through friction
        float staticFriction    = 0.05f;
        float dynamicFriction   = 0.05f;
        
        // Entity may want unlocked orientation (for kinematic motion)
        // but not want physics responses to cause orientation changes
        bool influenceOrientation = true;

        // ***** Collision Response methods *****
        // double dispatch other
        virtual void collision_response(I_PhysicsResponseComponent* otherBody, ColliderPacket& thisData, ColliderPacket& otherData, glm::vec3& collisionNormal, float& collisionDepth, glm::vec3& collisionPoint) override;

        // spring-spring response
        virtual void collision_response(SpringBodyComponent* otherSpringBody, ColliderPacket& thisData, ColliderPacket& otherData, glm::vec3& collisionNormal, float& collisionDepth, glm::vec3& collisionPoint) override;
        
        // spring-rigid response
        virtual void collision_response(RigidBodyComponent* otherRigidBody, ColliderPacket& thisData, ColliderPacket& otherData, glm::vec3& collisionNormal, float& collisionDepth, glm::vec3& collisionPoint) override;
    };

    // Virtual dispatch makes SpringBodyComponent non-POD, so we must override Message serialization
    template<typename T_Msg>
    Message<T_Msg>& operator<<(Message<T_Msg>& msg, const SpringBodyComponent& data)
    {
        // make sure stream operators are updated if members are updated
        static_assert(SpringBodyComponent::dataSize == 21, "SpringBodyComponent Message serializer found unexpected data size");

        // no I_PhysicsResponseComponent data
        //msg << ((I_PhysicsResponseComponent&)*data);

        uint32_t i = static_cast<uint32_t>(msg.size());
        // resize all at once
        msg.body.resize(msg.body.size() + SpringBodyComponent::dataSize);

        std::memcpy(msg.body.data() + i, &(data.restLength), sizeof(float));
        i += sizeof(float);
        
        std::memcpy(msg.body.data() + i, &(data.stiffness), sizeof(float));
        i += sizeof(float);

        std::memcpy(msg.body.data() + i, &(data.damping), sizeof(float));
        i += sizeof(float);
        
        std::memcpy(msg.body.data() + i, &(data.staticFriction), sizeof(float));
        i += sizeof(float);
        
        std::memcpy(msg.body.data() + i, &(data.dynamicFriction), sizeof(float));
        i += sizeof(float);
        
        std::memcpy(msg.body.data() + i, &(data.influenceOrientation), sizeof(bool));
        i += sizeof(bool);

        // recalc message size
        msg.header.size = static_cast<uint32_t>(msg.size());

        return msg;
    }
    template<typename T_Msg>
    Message<T_Msg>& operator>>(Message<T_Msg>& msg, SpringBodyComponent& data)
    {
        // stream out when no data is available;
        assert(msg.size() >= SpringBodyComponent::dataSize);
        
        // track index at the start of the data on "top" of the stack
        uint32_t i = static_cast<uint32_t>(msg.size()) - SpringBodyComponent::dataSize;

        std::memcpy(&(data.restLength), msg.body.data() + i, sizeof(float));
        i += sizeof(float);
        
        std::memcpy(&(data.stiffness), msg.body.data() + i, sizeof(float));
        i += sizeof(float);

        std::memcpy(&(data.damping), msg.body.data() + i, sizeof(float));
        i += sizeof(float);
        
        std::memcpy(&(data.staticFriction), msg.body.data() + i, sizeof(float));
        i += sizeof(float);
        
        std::memcpy(&(data.dynamicFriction), msg.body.data() + i, sizeof(float));
        i += sizeof(float);
        
        std::memcpy(&(data.influenceOrientation), msg.body.data() + i, sizeof(bool));
        i += sizeof(bool);
        
        // shrink, removing end of stack (constant time)
        msg.body.resize(msg.size() - SpringBodyComponent::dataSize);

        // recalc message size
        msg.header.size = static_cast<uint32_t>(msg.size());
        
        return msg;
    }
}

#endif // SPRING_BODY_COMPONENT_H