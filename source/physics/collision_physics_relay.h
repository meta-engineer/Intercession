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
            // unused for discrete collision detection
            UNREFERENCED_PARAMETER(deltaTime);

            for (std::vector<PhysicsPacket>::iterator thisPacket_it = m_physicsPackets.begin(); thisPacket_it != m_physicsPackets.end(); thisPacket_it++)
            {
                PhysicsPacket& thisData = *thisPacket_it;

                // no spacial partitioning :(
                for (std::vector<PhysicsPacket>::iterator otherPacket_it = thisPacket_it + 1; otherPacket_it != m_physicsPackets.end(); otherPacket_it++)
                {
                    assert(otherPacket_it != thisPacket_it);
                    PhysicsPacket& otherData = *otherPacket_it;

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
                    const float restitutionFactor    = 0.50f;       // energy retained along normal
                    const float staticFrictionCoeff  = 0.40f;       // max energy absorbed along tangent
                    const float dynamicFrictionCoeff = 0.30f;       // energy lost along tangent
                    

                    // STEP 2: static resolution
                    // collisionNormal is in direction away from "other", towards "this"
                    thisData.transform.origin  += collisionNormal * collisionDepth * (1-massFactor);
                    otherData.transform.origin -= collisionNormal * collisionDepth * massFactor;
                    collisionPoint             -= collisionNormal * collisionDepth * massFactor;

                    // STEP 3: geometry properties
                    // STEP 3.1: vector describing the "radius" of the rotation
                    const glm::vec3 thisLever = (collisionPoint - thisData.transform.origin);
                    const glm::vec3 otherLever = (collisionPoint - otherData.transform.origin);

                    PLEEPLOG_DEBUG("This lever: " + std::to_string(thisLever.x) + ", " + std::to_string(thisLever.y) + ", " + std::to_string(thisLever.z));
                    PLEEPLOG_DEBUG("Length of this lever: " + std::to_string(glm::length(thisLever)));

                    PLEEPLOG_DEBUG("Other lever: " + std::to_string(otherLever.x) + ", " + std::to_string(otherLever.y) + ", " + std::to_string(otherLever.z));
                    PLEEPLOG_DEBUG("Length of other lever: " + std::to_string(glm::length(otherLever)));

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
                    // relative is: this' velocity as viewed by other
                    const glm::vec3 relVelocity = ((thisData.physics.velocity + glm::cross(thisData.physics.angularVelocity, thisLever)) - (otherData.physics.velocity + glm::cross(otherData.physics.angularVelocity, otherLever)));
                    PLEEPLOG_DEBUG("Relative Velocity at collision: " + std::to_string(relVelocity.x) + ", " + std::to_string(relVelocity.y) + ", " + std::to_string(relVelocity.z));


                    // STEP 4: determine normal impulse
                    const float normalImpulse = (-1.0f * (1+restitutionFactor) * glm::dot(relVelocity, collisionNormal)) /
                        (otherInvMass + thisInvMass +
                            glm::dot(
                                glm::cross(otherInvMoment * glm::cross(otherLever, collisionNormal), otherLever) +
                                glm::cross(thisInvMoment * glm::cross(thisLever, collisionNormal), thisLever),
                                collisionNormal
                            )
                        );
                    PLEEPLOG_DEBUG("Determined Normal impulse to be: " + std::to_string(normalImpulse));

                    // Step 5: friction
                    // STEP 5.1: Determine velocity perpendicular to normal (tangent along surface)
                    const float normalAffinity = glm::dot(relVelocity, collisionNormal);
                    const glm::vec3 collisionTangent =  normalAffinity == 0.0f ? glm::vec3(0.0f) :
                        glm::normalize(relVelocity - normalAffinity * collisionNormal);
                    
                    PLEEPLOG_DEBUG("Collision Tangent: " + std::to_string(collisionTangent.x) + ", " + std::to_string(collisionTangent.y) + ", " + std::to_string(collisionTangent.z));

                    // STEP 5.2: Determine friction impulse
                    const float tangentImpulse = -1.0f * glm::dot(relVelocity, collisionTangent) /
                        (otherInvMass + thisInvMass +
                        glm::dot(
                            glm::cross(otherInvMoment * glm::cross(otherLever, collisionTangent), otherLever) + 
                            glm::cross(thisInvMoment * glm::cross(thisLever, collisionTangent), thisLever), 
                            collisionTangent)
                        );
                        
                    PLEEPLOG_DEBUG("Calced Friction impulse: " + std::to_string(tangentImpulse));
                    
                    // STEP 5.3: Coefficient factors
                    // if impulse is less than static max, then aply it (this should negate all colinear velocity)
                    // if impulse is greater than static max, multiply it by dynamic coefficient
                    //const float mu = 0.5f;
                    const float frictionCone = staticFrictionCoeff * normalImpulse;
                    PLEEPLOG_DEBUG("Static friction limit: " + std::to_string(frictionCone));

                    //const float frictionImpulse = std::abs(tangentImpulse) < std::abs(frictionCone) ? tangentImpulse : tangentImpulse * dynamicFrictionCoeff;
                    const float frictionImpulse = tangentImpulse * dynamicFrictionCoeff;
                    PLEEPLOG_DEBUG("Limited Friction impulse: " + std::to_string(frictionImpulse));

                    // STEP 6: Damping
                    // we have restitution/friction coefficients in impulses,
                    // but we may need extra damping to avoid stuttering and floating point errors
                    // Unfortunately it seems these kind of values need to be experimentally tweaked,
                    //   and no single solution works for all cases/scales
                    // only angular impulse really needs damping applied (See step 7.3)
                    
                    // "slop" damping
                    //const float flatDamping = 0.01f;

                    // static percentage damping
                    //const float staticDamping = 0.95f;

                    // exponential damping which is stronger approaching 0 relative velocity at collision point
                    //const float invDampingStrength = 32;
                    //const float dynamicDamping = calculate_damping(relVelocity, invDampingStrength);

                    // exponential damping relative to difference of angular velocity
                    //const float relativeAV2 = glm::length2(thisData.physics.angularVelocity - otherData.physics.angularVelocity);
                    //const float avDamping = -1.0f / (1.0f + relativeAV2 * invDampingStrength) + 1.0f;


                    // STEP 7: dynamic resolution
                    // STEP 7.1: resolve linear normal impulse response
                    thisData.physics.velocity  += thisInvMass * (normalImpulse*collisionNormal);
                    otherData.physics.velocity -= otherInvMass * (normalImpulse*collisionNormal);

                    // STEP 7.2 resolve linear friction impulse response
                    thisData.physics.velocity  += thisInvMass * (frictionImpulse*collisionTangent);
                    otherData.physics.velocity -= otherInvMass * (frictionImpulse*collisionTangent);

                    // STEP 7.3: resolve angular normal impulse response
                    const glm::vec3 thisAngularNormalImpulse  = thisInvMoment * glm::cross(thisLever, (normalImpulse*collisionNormal));
                    const glm::vec3 otherAngularNormalImpulse = otherInvMoment * glm::cross(otherLever, (normalImpulse*collisionNormal));
                    thisData.physics.angularVelocity  += thisAngularNormalImpulse;
                    otherData.physics.angularVelocity -= otherAngularNormalImpulse;
                    
                    PLEEPLOG_DEBUG("This Normal Angular Impulse: " + std::to_string(thisAngularNormalImpulse.x) + ", " + std::to_string(thisAngularNormalImpulse.y) + ", " + std::to_string(thisAngularNormalImpulse.z));
                    PLEEPLOG_DEBUG("Other Normal Angular Impulse: " + std::to_string(-otherAngularNormalImpulse.x) + ", " + std::to_string(-otherAngularNormalImpulse.y) + ", " + std::to_string(-otherAngularNormalImpulse.z));
                    

                    // STEP 7.4 resolve angular friction impulse response
                    const glm::vec3 thisAngularFrictionImpulse  = thisInvMoment * glm::cross(thisLever, (frictionImpulse*collisionTangent));
                    const glm::vec3 otherAngularFrictionImpulse = otherInvMoment * glm::cross(otherLever, (frictionImpulse*collisionTangent));
                    thisData.physics.angularVelocity  += thisAngularFrictionImpulse;
                    otherData.physics.angularVelocity -= otherAngularFrictionImpulse;
                    
                    PLEEPLOG_DEBUG("This Friction Angular Impulse: " + std::to_string(thisAngularFrictionImpulse.x) + ", " + std::to_string(thisAngularFrictionImpulse.y) + ", " + std::to_string(thisAngularFrictionImpulse.z));
                    PLEEPLOG_DEBUG("Other Friction Angular Impulse: " + std::to_string(-otherAngularFrictionImpulse.x) + ", " + std::to_string(-otherAngularFrictionImpulse.y) + ", " + std::to_string(-otherAngularFrictionImpulse.z));


                    const glm::vec3 postRelVelocity = (thisData.physics.velocity + glm::cross(thisData.physics.angularVelocity, thisLever) - otherData.physics.velocity - glm::cross(otherData.physics.angularVelocity, otherLever));
                    PLEEPLOG_DEBUG("Relative Velocity after collision: " + std::to_string(postRelVelocity.x) + ", " + std::to_string(postRelVelocity.y) + ", " + std::to_string(postRelVelocity.z));

                }
            }
        }

        static float calculate_damping(glm::vec3 vector, float invFactor)
        {
            return -1.0f / (1.0f +
                (vector.x*vector.x 
                + vector.y*vector.y
                + vector.z*vector.z) * invFactor) + 1.0f;
        }

        
        // store in a simple queue for now
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
        // TODO: hey dumb dumb figure out RTrees
        std::vector<PhysicsPacket> m_physicsPackets;
    };
}

#endif // COLLISION_PHYSICS_RELAY_H