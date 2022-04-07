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
                PhysicsPacket thisData = m_physicsPacketVector.back();
                m_physicsPacketVector.pop_back();

                // no spacial partitioning :(
                for (auto& otherData : m_physicsPacketVector)
                {
                    // STEP 0: Get collision data
                    glm::vec3 collisionNormal;
                    float collisionDepth;
                    glm::vec3 collisionPoint;
                    // Colliders double dispatch to their true type
                    // we can change intersection algorithm at runtime with a static collider member
                    if (!(thisData.physics.collider->intersects(otherData.physics.collider.get(), thisData.transform, otherData.transform, collisionNormal, collisionDepth, collisionPoint)))
                    {
                        continue;
                    }
                    PLEEPLOG_DEBUG("Collision Detected!");
                    PLEEPLOG_DEBUG("Collision Point: " + std::to_string(collisionPoint.x) + ", " + std::to_string(collisionPoint.y) + ", " + std::to_string(collisionPoint.z));
                    PLEEPLOG_DEBUG("Collision Normal: " + std::to_string(collisionNormal.x) + ", " + std::to_string(collisionNormal.y) + ", " + std::to_string(collisionNormal.z));
                    PLEEPLOG_DEBUG("Collision Depth: " + std::to_string(collisionDepth));
                    PLEEPLOG_DEBUG("This @: " + std::to_string(thisData.transform.origin.x) + ", " + std::to_string(thisData.transform.origin.y) + ", " + std::to_string(thisData.transform.origin.z));
                    PLEEPLOG_DEBUG("Other @: " + std::to_string(otherData.transform.origin.x) + ", " + std::to_string(otherData.transform.origin.y) + ", " + std::to_string(otherData.transform.origin.z));
                    

                    // STEP 1: Fetch entity properties
                    // STEP 1.1: material physics properties
                    float massFactor;
                    bool thisImmutable = thisData.physics.mass == INFINITE_MASS || !thisData.physics.isDynamic;
                    bool otherImmutable = otherData.physics.mass == INFINITE_MASS || !otherData.physics.isDynamic;
                    if (thisImmutable && otherImmutable)
                        continue;   // no possible resolution
                    else if (thisImmutable)
                        massFactor = 1;
                    else if (otherImmutable)
                        massFactor = 0;
                    else
                        massFactor = thisData.physics.mass/(thisData.physics.mass + otherData.physics.mass);
                    PLEEPLOG_DEBUG("MassFactor of this: " + std::to_string(massFactor));
                    
                    float thisInvMass = 0;
                    if (!thisImmutable)
                    {
                        thisInvMass = 1.0f/thisData.physics.mass;
                    }
                    PLEEPLOG_DEBUG("this inverse mass: " + std::to_string(thisInvMass));
                    float otherInvMass = 0;
                    if (!otherImmutable)
                    {
                        otherInvMass = 1.0f/otherData.physics.mass;
                    }
                    PLEEPLOG_DEBUG("other inverse mass: " + std::to_string(otherInvMass));
                    
                    // STEP 1.2: material collision properties
                    // TODO: fetch from physics attributes of both objects
                    const float frictionFactor = 0.99f;         // energy retained along tangent
                    const float restitutionFactor = 0.50f;      // energy retained along normal
                    
                    // STEP 2: static resolution
                    // collisionNormal is in direction away from "other", towards "this"
                    thisData.transform.origin  += collisionNormal * collisionDepth * (1-massFactor);
                    otherData.transform.origin -= collisionNormal * collisionDepth * massFactor;
                    collisionPoint             -= collisionNormal * collisionDepth * massFactor;

                    // STEP 3: geometry properties
                    // STEP 3.1: vector describing the "radius" of the rotation
                    const glm::vec3 thisLever = collisionPoint - thisData.transform.origin;
                    const glm::vec3 otherLever = collisionPoint - otherData.transform.origin;

                    // STEP 3.2: angular inertia/moment
                    // TODO: can this be optimized? inverse of inverse :(
                    const glm::mat3 thisInverseModel = glm::inverse(glm::mat3(thisData.transform.get_model_transform()));
                    const glm::mat3 thisInvMoment = thisImmutable ? glm::mat3(0.0f) 
                        : glm::inverse(
                            glm::transpose(thisInverseModel) 
                            * (thisData.physics.collider->getInertiaTensor() * thisData.physics.mass)
                            * thisInverseModel
                        );

                    const glm::mat3 otherInverseModel = glm::inverse(glm::mat3(otherData.transform.get_model_transform()));
                    const glm::mat3 otherInvMoment = otherImmutable ? glm::mat3(0.0f)
                        : glm::inverse(
                            glm::transpose(otherInverseModel)
                            * (otherData.physics.collider->getInertiaTensor() * otherData.physics.mass)
                            * otherInverseModel
                        );

                    // STEP 3.3 relative velocity vector
                    const glm::vec3 relVelocity = (thisData.physics.velocity + glm::cross(thisData.physics.angularVelocity, thisLever)) - (otherData.physics.velocity + glm::cross(otherData.physics.angularVelocity, otherLever));
                    PLEEPLOG_DEBUG("Relative Velocity at collision: " + std::to_string(relVelocity.x) + ", " + std::to_string(relVelocity.y) + ", " + std::to_string(relVelocity.z));


                    // STEP 4: determine impulse
                    const float impulse = (-1.0f * (1+restitutionFactor) * glm::dot(relVelocity, collisionNormal)) /
                        (otherInvMass + thisInvMass 
                            + glm::dot(
                                glm::cross(otherInvMoment * glm::cross(otherLever, collisionNormal), otherLever)
                                + glm::cross(thisInvMoment * glm::cross(thisLever, collisionNormal), thisLever),
                                collisionNormal
                            )
                        );
                    PLEEPLOG_DEBUG("Determined impulse factor to be: " + std::to_string(impulse));

                    // STEP 5: exponential damping which is stronger approaching 0 velocity (stops jittering)
                    // Determine component of velocities parallel to collision normal
/*
                    // Assuming normals are length 1 we don't need to divide dot product by length
                    const glm::vec3 thisNormalVelocity = glm::dot(thisData.physics.velocity, collisionNormal) * collisionNormal;
                    const glm::vec3 otherNormalVelocity = glm::dot(otherData.physics.velocity, collisionNormal) * collisionNormal;

                    const float thisDamping = -1.0f / (1.0f +
                        thisNormalVelocity.x*thisNormalVelocity.x 
                        + thisNormalVelocity.y*thisNormalVelocity.y
                        + thisNormalVelocity.z*thisNormalVelocity.z * 16) + 1.0f;
                        
                    const float otherDamping = -1.0f / (1.0f +
                        otherNormalVelocity.x*otherNormalVelocity.x 
                        + otherNormalVelocity.y*otherNormalVelocity.y
                        + otherNormalVelocity.z*otherNormalVelocity.z * 16) + 1.0f;

                    UNREFERENCED_PARAMETER(thisDamping);
                    UNREFERENCED_PARAMETER(otherDamping);
*/
                    // STEP 6: dynamic resolution
                    // STEP 6.1: resolve linear impulse response
                    thisData.physics.velocity  += (impulse*thisInvMass*collisionNormal);
                    otherData.physics.velocity -= (impulse*otherInvMass*collisionNormal);

                    const glm::vec3 postRelVelocity = (thisData.physics.velocity + glm::cross(thisData.physics.angularVelocity, thisLever) - otherData.physics.velocity - glm::cross(otherData.physics.angularVelocity, otherLever));
                    PLEEPLOG_DEBUG("Relative Velocity after collision: " + std::to_string(postRelVelocity.x) + ", " + std::to_string(postRelVelocity.y) + ", " + std::to_string(postRelVelocity.z));

                    // STEP 6.2: resolve angular impulse response
                    glm::vec3 thisAngularImpulse  = impulse*thisInvMoment*glm::cross(thisLever, collisionNormal);
                    glm::vec3 otherAngularImpulse = impulse*otherInvMoment*glm::cross(otherLever, collisionNormal);
                    
                    thisData.physics.angularVelocity += thisAngularImpulse;
                    otherData.physics.angularVelocity -= otherAngularImpulse;

                    PLEEPLOG_DEBUG("This Angular Impulse: " + std::to_string(thisAngularImpulse.x) + ", " + std::to_string(thisAngularImpulse.y) + ", " + std::to_string(thisAngularImpulse.z));
                    PLEEPLOG_DEBUG("Other Angular Impulse: " + std::to_string(otherAngularImpulse.x) + ", " + std::to_string(otherAngularImpulse.y) + ", " + std::to_string(otherAngularImpulse.z));
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