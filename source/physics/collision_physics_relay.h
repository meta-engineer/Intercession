#ifndef COLLISION_PHYSICS_RELAY_H
#define COLLISION_PHYSICS_RELAY_H

//#include "intercession_pch.h"
#include <vector>

#include "physics/i_physics_relay.h"
#include "logging/pleep_log.h"

namespace pleep
{
    class CollisionPhysicsRelay : public IPhysicsRelay
    {
    public:
        // motion integration should already have happened
        // test for collision, do static resolution, do dynamic resolution
        void engage(double deltaTime) override
        {
            // if deltaTime is too large, we may have to detect and split into
            // multiple passes. (or if we just want to increase accuracy)

            while (!m_physicsPacketVector.empty())
            {
                // vector is FIFO unlike queue
                PhysicsPacket data = m_physicsPacketVector.back();
                m_physicsPacketVector.pop_back();
                
                // no spacial partitioning :(
                for (auto& otherData : m_physicsPacketVector)
                {
                    if (data.physics.collider->intersects(otherData.physics.collider))
                    {
                        // resolution
                        UNREFERENCED_PARAMETER(deltaTime);
                    }
                }
            }
        }

        
        // store in a simple queue for now
        void submit(PhysicsPacket data) override
        {
            m_physicsPacketVector.push_back(data);
        }

    private:
        // TODO: hey dumb dumb figure out RTrees
        std::vector<PhysicsPacket> m_physicsPacketVector;
    };
}

#endif // COLLISION_PHYSICS_RELAY_H