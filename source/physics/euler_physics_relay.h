#ifndef EULER_PHYSICS_RELAY_H
#define EULER_PHYSICS_RELAY_H

//#include "intercession_pch.h"
#include <queue>

#include "physics/i_physics_relay.h"
#include "logging/pleep_log.h"

namespace pleep
{
    // This is "Improved Euler" method for motion integration on entities with physics
    class EulerPhysicsRelay : public IPhysicsRelay
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
                data.physics.acceleration += glm::vec3(0.0f, -9.8f, 0.0f);

                // half-step
                data.transform.origin += data.physics.velocity * (float)(deltaTime / 2);
                data.transform.rotation += data.physics.eulerVelocity * (float)(deltaTime / 2);

                // apply acceleration
                data.physics.velocity += data.physics.acceleration * (float)deltaTime;
                data.physics.eulerVelocity += data.physics.eulerAcceleration * (float)deltaTime;

                // finish step
                data.transform.origin += data.physics.velocity * (float)(deltaTime / 2);
                data.transform.rotation += data.physics.eulerVelocity * (float)(deltaTime / 2);

                // Should we clear acceleration here
                // or leave it for other relays to use?
                data.physics.acceleration = glm::vec3(0.0f);
                data.physics.eulerAcceleration = glm::vec3(0.0f);
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

#endif // EULER_PHYSICS_RELAY_H