#ifndef EULER_PHYSICS_RELAY_H
#define EULER_PHYSICS_RELAY_H

//#include "intercession_pch.h"
#include <vector>

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
            for (std::vector<PhysicsPacket>::iterator packet_it = m_physicsPackets.begin(); packet_it != m_physicsPackets.end(); packet_it++)
            {
                PhysicsPacket& data = *packet_it;

                // ensure non-Dynamic Objects never move, or accidentally accumulate velocities/accelerations
                if (!data.physics.isDynamic)
                {
                    data.physics.velocity            = glm::vec3(0.0f);
                    data.physics.angularVelocity     = glm::vec3(0.0f);
                    data.physics.acceleration        = glm::vec3(0.0f);
                    data.physics.angularAcceleration = glm::vec3(0.0f);
                    continue;
                }

                // SHHH... temporary global gravity
                data.physics.acceleration += glm::vec3(0.0f, -9.8f, 0.0f);
                // Global air drag?
                //data.physics.velocity *= 0.99;
                //data.physics.angularVelocity *= 0.95;

                // half-step
                data.transform.origin += data.physics.velocity * (float)(deltaTime / 2.0f);
                // calculate angular speed
                float angularSpeed = glm::length(data.physics.angularVelocity);
                glm::quat quatVelocity = angularSpeed == 0 ? glm::quat(glm::vec3(0.0f)) :
                    glm::angleAxis(angularSpeed * (float)(deltaTime / 2.0f), data.physics.angularVelocity / angularSpeed);
                data.transform.orientation = quatVelocity * data.transform.orientation;

                // apply acceleration
                data.physics.velocity += data.physics.acceleration * (float)deltaTime;
                data.physics.angularVelocity += data.physics.angularAcceleration * (float)deltaTime;

                // finish half-step
                data.transform.origin += data.physics.velocity * (float)(deltaTime / 2.0f);
                // re-generate accelerated angular velocity
                angularSpeed = glm::length(data.physics.angularVelocity);
                quatVelocity = angularSpeed == 0 ? glm::quat(glm::vec3(0.0f)) :
                    glm::angleAxis(angularSpeed * (float)(deltaTime / 2.0f), data.physics.angularVelocity / angularSpeed);
                data.transform.orientation = quatVelocity * data.transform.orientation;

                // Should we clear acceleration here
                // or leave it for other relays to use?
                data.physics.acceleration = glm::vec3(0.0f);
                data.physics.angularAcceleration = glm::vec3(0.0f);
            }
        }
        
        void submit(PhysicsPacket data) override
        {
            m_physicsPackets.push_back(data);
        }

        // clear packets for next frame
        void clear() override
        {
            m_physicsPackets.clear();
        }

    private:
        // becuase of fixed timestep we may need to process packets multiple times, so vector
        std::vector<PhysicsPacket> m_physicsPackets;
    };
}

#endif // EULER_PHYSICS_RELAY_H