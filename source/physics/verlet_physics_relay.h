#ifndef VERLET_PHYSICS_RELAY_H
#define VERLET_PHYSICS_RELAY_H

//#include "intercession_pch.h"
#include <queue>

#include "physics/i_physics_relay.h"
#include "logging/pleep_log.h"

namespace pleep
{
    class VerletPhysicsRelay : public IPhysicsRelay
    {
    public:
        // consume acceleration values and step motion integration forward
        void engage(double deltaTime) override
        {
            // if deltaTime is too large, we may have to detect and split into
            // multiple passes. (or if we just want to increase accuracy)

            while (!m_physicsPacketQueue.empty())
            {
                PhysicsPacket data = m_physicsPacketQueue.front();
                m_physicsPacketQueue.pop();

                if (!data.physics.isDynamic) continue;

                // SHHH... temporary global gravity
                //data.physics.acceleration += glm::vec3(0.0f, -9.8f, 0.0f);
                
                // half-step
                data.transform.origin += data.physics.velocity * (float)(deltaTime / 2);
                data.transform.rotation += data.physics.angularVelocity * (float)(deltaTime / 2);

                // apply acceleration
                data.physics.velocity += data.physics.acceleration * (float)deltaTime;
                data.physics.angularVelocity += data.physics.angularAcceleration * (float)deltaTime;

                // finish step
                data.transform.origin += data.physics.velocity * (float)(deltaTime / 2);
                data.transform.rotation += data.physics.angularVelocity * (float)(deltaTime / 2);

                // Should we clear acceleration here
                // or leave it for other relays to use?
                data.physics.acceleration = glm::vec3(0.0f);
                data.physics.angularAcceleration = glm::vec3(0.0f);
            }
        }
        
        // store in a simple queue for now
        void submit(PhysicsPacket data) override
        {
            m_physicsPacketQueue.push(data);
        }

    private:
        std::queue<PhysicsPacket> m_physicsPacketQueue;
    };
}

#endif // VERLET_PHYSICS_RELAY_H