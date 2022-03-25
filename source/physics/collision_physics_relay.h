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
            UNREFERENCED_PARAMETER(deltaTime);

            while (!m_physicsPacketVector.empty())
            {
                // vector is FIFO unlike queue
                PhysicsPacket data = m_physicsPacketVector.back();
                m_physicsPacketVector.pop_back();

                // no spacial partitioning :(
                for (auto& otherData : m_physicsPacketVector)
                {
                    // Get collision data
                    glm::vec3 collisionNormal;
                    float collisionDepth;
                    // Colliders double dispatch to their true type
                    // we can change intersection algorithm at runtime with a static collider member
                    if (!(data.physics.collider->intersects(otherData.physics.collider.get(), data.transform, otherData.transform, collisionNormal, collisionDepth)))
                    {
                        continue;
                    }
                    
                    //PLEEPLOG_DEBUG("Collision Detected!");
                    //PLEEPLOG_DEBUG(std::to_string(collisionNormal.x) + ", " + std::to_string(collisionNormal.y) + ", " + std::to_string(collisionNormal.z) + " @ " + std::to_string(collisionDepth));

                    // proportion of response of this mass : other mass
                    float massFactor;
                    bool thisImmutable = data.physics.mass == INFINITE_MASS 
                        || !data.physics.isDynamic;
                    bool otherImmutable = otherData.physics.mass == INFINITE_MASS 
                        || !otherData.physics.isDynamic;
                    
                    if (thisImmutable && otherImmutable)
                        continue;   // no possible resolution
                    else if (thisImmutable)
                        massFactor = 1;
                    else if (otherImmutable)
                        massFactor = 0;
                    else
                        massFactor = data.physics.mass/(data.physics.mass + otherData.physics.mass);
                    
                    // global properties
                    // TODO: fetch from physics attributes of both objects
                    const float frictionFactor = 0.99f;         // energy retained along tangent
                    const float restitutionFactor = 0.99f;      // energy retained along normal

                    // STEP 1: static resolution
                    // collisionNormal is in direction away from "other"
                    data.transform.origin      += collisionNormal * collisionDepth * (1-massFactor);
                    otherData.transform.origin -= collisionNormal * collisionDepth * massFactor;

                    // STEP 2: dynamic resolution
                    // we need location of collision?, collision normal, and depth
                    // dot product between collision normal and (centre of mass - collision location)
                    // should indicate how much of the delta should be linear and how much is angular

                    // STEP 2.1: Determine component of velocities parallel to collision normal
                    // Assuming normals are length 1 we don't need to divide dot product by length
                    glm::vec3 thisNormalVelocity = glm::dot(data.physics.velocity, collisionNormal) * collisionNormal;
                    glm::vec3 otherNormalVelocity = glm::dot(otherData.physics.velocity, collisionNormal) * collisionNormal;

                    // STEP 2.2: determine proportion of linear vs. angular "force"
                    // need angle between collision vector and vector from centre of mass to collision location (the "radius" of the applied torque)
                    //glm::vec3 lever = glm::vec3(1.0f);
                    //float linearFactor = cos(angle)
                    //float angularFactor = sin(angle)
                    // angular energy can become linear and visa-versa... how?

                    // STEP 2.2: resolve linear momentum
                    // seperate tangent velocity
                    data.physics.velocity -= thisNormalVelocity;
                    // apply friction along tangent velocity
                    data.physics.velocity *= frictionFactor;
                    // conservation of momentum using only components parallel to collision normal
                    glm::vec3 thisCollidedVelocity = (2*massFactor - 1) * thisNormalVelocity + (2* (1-massFactor)) * otherNormalVelocity;
                    // exponential damping which is stronger approaching 0 velocity (stops jittering)
                    float thisDamping = -1.0f / (1.0f +
                        thisCollidedVelocity.x*thisCollidedVelocity.x 
                        + thisCollidedVelocity.y*thisCollidedVelocity.y
                        + thisCollidedVelocity.z*thisCollidedVelocity.z * 16) + 1.0f;
                    // restitution sets asymptotic "max" energy retained for damping at highest velocities
                    data.physics.velocity += thisCollidedVelocity * thisDamping * restitutionFactor;

                    // repeat for other
                    otherData.physics.velocity -= otherNormalVelocity;
                    otherData.physics.velocity *= frictionFactor;
                    glm::vec3 otherCollidedVelocity = (2*massFactor) * thisNormalVelocity + (1 - 2*massFactor) * otherNormalVelocity;
                    float otherDamping = -1.0f / (1.0f +
                        otherCollidedVelocity.x*otherCollidedVelocity.x 
                        + otherCollidedVelocity.y*otherCollidedVelocity.y
                        + otherCollidedVelocity.z*otherCollidedVelocity.z * 16) + 1.0f;
                    otherData.physics.velocity += otherCollidedVelocity * otherDamping * restitutionFactor;

                    // STEP 2.3: resolve angular momentum

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