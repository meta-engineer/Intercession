#ifndef EULER_PHYSICS_RELAY_H
#define EULER_PHYSICS_RELAY_H

//#include "intercession_pch.h"
#include <vector>

#include "logging/pleep_log.h"
#include "physics/a_physics_relay.h"
#include "physics/physics_packet.h"

namespace pleep
{
    // This is "Improved Euler" method for motion integration on entities with physics
    class EulerPhysicsRelay : public A_PhysicsRelay
    {
    public:
        // explicitly inherit constructors
        using A_PhysicsRelay::A_PhysicsRelay;

        // consume acceleration values and step motion integration forward
        void engage(double deltaTime) override
        {
            for (std::vector<PhysicsPacket>::iterator packet_it = m_physicsPackets.begin(); packet_it != m_physicsPackets.end(); packet_it++)
            {
                PhysicsPacket& data = *packet_it;

                // skip, but don't clear sleeping objects
                if (data.physics.isAsleep) continue;
                
                ////////////////////////////////////////////////////////////////
                // SHHH... temporary global gravity                           //
                data.physics.acceleration += glm::vec3(0.0f, -9.8f, 0.0f);    //
                ////////////////////////////////////////////////////////////////

                // Apply locking constraints
                if (data.physics.lockOrigin)
                {
                    data.physics.velocity            = glm::vec3(0.0f);
                    data.physics.acceleration        = glm::vec3(0.0f);
                    // to compensate for static collision resolution (or anything else)
                    // apply constraint position exactly
                    data.transform.origin = data.physics.lockedOrigin;

                    // or integrate/interpolate towards locked position
                    //data.transform.origin += (data.physics.lockedOrigin - data.transform.origin) * (float)deltaTime;
                }
                if (data.physics.lockOrientation)
                {
                    data.physics.angularVelocity     = glm::vec3(0.0f);
                    data.physics.angularAcceleration = glm::vec3(0.0f);
                    // static collision resolution does not effect orientation, but other systems could
                    // apply constraint orientation exactly
                    data.transform.orientation = data.physics.lockedOrientation;

                    // or integrate/interpolate towards locked orientation?
                    //data.transform.orientation = glm::mix(data.transform.orientation, data.physics.lockedOrientation, std::min((float)deltaTime, 1.0f));
                }

                // half-step
                data.transform.origin += data.physics.velocity * (float)(deltaTime / 2.0f);
                // calculate angular speed
                if (data.physics.angularVelocity != glm::vec3(0.0f))
                {
                    float angularSpeed = glm::length(data.physics.angularVelocity);
                    glm::quat quatVelocity = glm::angleAxis(angularSpeed * (float)(deltaTime / 2.0f), data.physics.angularVelocity / angularSpeed);
                    data.transform.orientation = quatVelocity * data.transform.orientation;
                }

                // apply acceleration
                data.physics.velocity += data.physics.acceleration * (float)deltaTime;
                data.physics.angularVelocity += data.physics.angularAcceleration * (float)deltaTime;

                // finish half-step
                data.transform.origin += data.physics.velocity * (float)(deltaTime / 2.0f);
                if (data.physics.angularVelocity != glm::vec3(0.0f))
                {
                    float angularSpeed = glm::length(data.physics.angularVelocity);
                    glm::quat quatVelocity = glm::angleAxis(angularSpeed * (float)(deltaTime / 2.0f), data.physics.angularVelocity / angularSpeed);
                    data.transform.orientation = quatVelocity * data.transform.orientation;
                }

                // leave acceleration values as-is for next (potential) engage() call?
                // clear accelerations from last frame
                data.physics.acceleration        = glm::vec3(0.0f);
                data.physics.angularAcceleration = glm::vec3(0.0f);
                
                // Per-step drag
                data.physics.velocity        *= 1.0f - data.physics.linearDrag * deltaTime;
                data.physics.angularVelocity *= 1.0f - data.physics.angularDrag * deltaTime;

            }
        }
        
        void submit(PhysicsPacket data)
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