#ifndef COLLISION_PHYSICS_RELAY_H
#define COLLISION_PHYSICS_RELAY_H

//#include "intercession_pch.h"
#include <vector>

#include "logging/pleep_log.h"
#include "physics/i_physics_relay.h"
#include "physics/collider_packet.h"
#include "physics/physics_component.h"
#include "core/cosmos.h"

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

            for (std::vector<ColliderPacket>::iterator thisPacket_it = m_colliderPackets.begin(); thisPacket_it != m_colliderPackets.end(); thisPacket_it++)
            {
                ColliderPacket& thisData = *thisPacket_it;
                if (thisData.collider->get_type() == ColliderType::none)
                    continue;

                // no spacial partitioning :(
                for (std::vector<ColliderPacket>::iterator otherPacket_it = thisPacket_it + 1; otherPacket_it != m_colliderPackets.end(); otherPacket_it++)
                {
                    assert(otherPacket_it != thisPacket_it);
                    ColliderPacket& otherData = *otherPacket_it;

                    // check other collider type (for removing double-dispatch later)
                    if (otherData.collider->get_type() == ColliderType::none)
                        continue;

                    // STEP 0: Get collision data
                    glm::vec3 collisionNormal;
                    float collisionDepth;
                    glm::vec3 collisionPoint;
                    // Colliders double dispatch to their true type
                    // we could change intersection algorithm at runtime with a static collider member
                    if (!(thisData.collider->static_intersect(otherData.collider, thisData.transform, otherData.transform, collisionNormal, collisionDepth, collisionPoint)))
                    {
                        continue;
                    }
                    //PLEEPLOG_DEBUG("Collision Detected!");
                    //PLEEPLOG_DEBUG("Collision Point: " + std::to_string(collisionPoint.x) + ", " + std::to_string(collisionPoint.y) + ", " + std::to_string(collisionPoint.z));
                    //PLEEPLOG_DEBUG("Collision Normal: " + std::to_string(collisionNormal.x) + ", " + std::to_string(collisionNormal.y) + ", " + std::to_string(collisionNormal.z));
                    //PLEEPLOG_DEBUG("Collision Depth: " + std::to_string(collisionDepth));
                    //PLEEPLOG_DEBUG("This @: " + std::to_string(thisData.transform.origin.x) + ", " + std::to_string(thisData.transform.origin.y) + ", " + std::to_string(thisData.transform.origin.z));
                    //PLEEPLOG_DEBUG("Other @: " + std::to_string(otherData.transform.origin.x) + ", " + std::to_string(otherData.transform.origin.y) + ", " + std::to_string(otherData.transform.origin.z));

                    // Now we need to forward these collisions to be resolved according to the collider's
                    // response type. ALSO, that may require fetching another component
                    
                    // For now we'll always do rigid body response
                    // TODO: Make this exception safe! colliders without physics components will throw!
                    PhysicsComponent& thisPhysics = thisData.owner->get_component<PhysicsComponent>(thisData.collidee);
                    PhysicsComponent& otherPhysics = otherData.owner->get_component<PhysicsComponent>(otherData.collidee);

                    // STEP 1: Fetch entity properties
                    // STEP 1.1: material physics properties
                    float massFactor;
                    // TODO: use collider's resolution type instead of sleep state
                    bool thisImmutable = thisPhysics.mass == INFINITE_MASS || thisPhysics.isAsleep;
                    bool otherImmutable = otherPhysics.mass == INFINITE_MASS || otherPhysics.isAsleep;
                    if (thisImmutable && otherImmutable)
                        continue;   // no possible resolution
                    else if (thisImmutable)
                        massFactor = 1;
                    else if (otherImmutable)
                        massFactor = 0;
                    else
                        massFactor = thisPhysics.mass/(thisPhysics.mass + otherPhysics.mass);
                    //PLEEPLOG_DEBUG("MassFactor of this: " + std::to_string(massFactor));
                    
                    float thisInvMass = 0;
                    if (!thisImmutable)
                    {
                        thisInvMass = 1.0f/thisPhysics.mass;
                    }
                    //PLEEPLOG_DEBUG("this inverse mass: " + std::to_string(thisInvMass));
                    float otherInvMass = 0;
                    if (!otherImmutable)
                    {
                        otherInvMass = 1.0f/otherPhysics.mass;
                    }
                    //PLEEPLOG_DEBUG("other inverse mass: " + std::to_string(otherInvMass));
                    
                    // STEP 1.2: material collision properties
                    // TODO: fetch from physics attributes of both objects
                    const float restitutionFactor    = 0.50f;       // energy retained along normal
                    const float staticFrictionCoeff  = 0.40f;       // max energy absorbed along tangent
                    const float dynamicFrictionCoeff = 0.30f;       // energy lost along tangent
                    

                    // STEP 2: static resolution
                    // TODO: static resolution can cause objects in complex scenarios to "walk around" and not
                    //   abide by the stability of the dynamic resolution, this can only be fixed with
                    //   a continuous intersect detection and continuous resolution, so its a major refactor
                    // collisionNormal is in direction away from "other", towards "this"

                    // STEP 2.1: resolve transform origin and collision point (on other)
                    thisData.transform.origin  += collisionNormal * collisionDepth * (1-massFactor);
                    otherData.transform.origin -= collisionNormal * collisionDepth * massFactor;
                    collisionPoint             -= collisionNormal * collisionDepth * massFactor;

                    // STEP 2.2: regenerate centre of masses after static resolution
                    // TODO: for a compound collider this will be more involved
                    //   for now take entity origin + collider origin?
                    const glm::vec3 thisCentreOfMass = thisData.transform.origin;
                    const glm::vec3 otherCentreOfMass = otherData.transform.origin;

                    // STEP 3: geometry properties
                    // STEP 3.1: vector describing the "radius" of the rotation
                    const glm::vec3 thisLever = (collisionPoint - thisCentreOfMass);
                    const glm::vec3 otherLever = (collisionPoint - otherCentreOfMass);

                    //PLEEPLOG_DEBUG("This lever: " + std::to_string(thisLever.x) + ", " + std::to_string(thisLever.y) + ", " + std::to_string(thisLever.z));
                    //PLEEPLOG_DEBUG("Length of this lever: " + std::to_string(glm::length(thisLever)));

                    //PLEEPLOG_DEBUG("Other lever: " + std::to_string(otherLever.x) + ", " + std::to_string(otherLever.y) + ", " + std::to_string(otherLever.z));
                    //PLEEPLOG_DEBUG("Length of other lever: " + std::to_string(glm::length(otherLever)));

                    // STEP 3.2: angular inertia/moment
                    // TODO: can this be optimized? inverse of inverse :(
                    // TODO: moment doesn't behave correct with scaled transforms
                    //   (and maybe also collider offset transforms)
                    const glm::mat3 thisInverseModel = glm::inverse(glm::mat3(thisData.transform.get_model_transform()));
                    const glm::mat3 thisInvMoment = thisImmutable ? glm::mat3(0.0f) 
                        : glm::inverse(
                            glm::transpose(thisInverseModel) 
                            * (thisData.collider->get_inertia_tensor() * thisPhysics.mass)
                            * thisInverseModel
                        );

                    const glm::mat3 otherInverseModel = glm::inverse(glm::mat3(otherData.transform.get_model_transform()));
                    const glm::mat3 otherInvMoment = otherImmutable ? glm::mat3(0.0f)
                        : glm::inverse(
                            glm::transpose(otherInverseModel)
                            * (otherData.collider->get_inertia_tensor() * otherPhysics.mass)
                            * otherInverseModel
                        );

                    // STEP 3.3 relative velocity vector
                    // relative is: this' velocity as viewed by other
                    const glm::vec3 relVelocity = ((thisPhysics.velocity + glm::cross(thisPhysics.angularVelocity, thisLever)) - (otherPhysics.velocity + glm::cross(otherPhysics.angularVelocity, otherLever)));
                    //PLEEPLOG_DEBUG("Relative Velocity at collision: " + std::to_string(relVelocity.x) + ", " + std::to_string(relVelocity.y) + ", " + std::to_string(relVelocity.z));


                    // STEP 4: determine normal impulse
                    const float normalImpulse = (-1.0f * (1+restitutionFactor) * glm::dot(relVelocity, collisionNormal)) /
                        (otherInvMass + thisInvMass +
                            glm::dot(
                                glm::cross(otherInvMoment * glm::cross(otherLever, collisionNormal), otherLever) +
                                glm::cross(thisInvMoment * glm::cross(thisLever, collisionNormal), thisLever),
                                collisionNormal
                            )
                        );

                    const float contactImpulse = (normalImpulse);
                    //PLEEPLOG_DEBUG("Calculated Contact impulse to be: " + std::to_string(contactImpulse));

                    // Step 5: friction
                    // STEP 5.1: Determine velocity perpendicular to normal (tangent along surface)
                    const float normalAffinity = glm::dot(relVelocity, collisionNormal);
                    const glm::vec3 collisionTangent =  normalAffinity == 0.0f ? glm::vec3(0.0f) :
                        glm::normalize(relVelocity - normalAffinity * collisionNormal);
                    
                    //PLEEPLOG_DEBUG("Collision Tangent: " + std::to_string(collisionTangent.x) + ", " + std::to_string(collisionTangent.y) + ", " + std::to_string(collisionTangent.z));

                    // STEP 5.2: Determine friction impulse
                    const float tangentImpulse = -1.0f * glm::dot(relVelocity, collisionTangent) /
                        (otherInvMass + thisInvMass +
                        glm::dot(
                            glm::cross(otherInvMoment * glm::cross(otherLever, collisionTangent), otherLever) + 
                            glm::cross(thisInvMoment * glm::cross(thisLever, collisionTangent), thisLever), 
                            collisionTangent)
                        );
                        
                    //PLEEPLOG_DEBUG("Calculated Friction impulse: " + std::to_string(tangentImpulse));
                    
                    // STEP 5.3: Coefficient factors
                    // if impulse is less than static max, then aply it (this should negate all colinear velocity)
                    // if impulse is greater than static max, multiply it by dynamic coefficient
                    const float frictionCone = staticFrictionCoeff * contactImpulse;
                    //PLEEPLOG_DEBUG("Static friction limit: " + std::to_string(frictionCone));

                    const float frictionImpulse = std::abs(tangentImpulse) < std::abs(frictionCone) ? tangentImpulse : tangentImpulse * dynamicFrictionCoeff;
                    //PLEEPLOG_DEBUG("Limited Friction impulse: " + std::to_string(frictionImpulse));

                    // STEP 6: Damping
                    // we have restitution/friction coefficients in impulses,
                    // but we may need extra damping to avoid stuttering and floating point errors
                    // Unfortunately it seems these kind of values need to be experimentally tweaked,
                    //   and no single solution works for all cases/scales
                    // only angular impulse really needs damping applied (See step 7.5)

                    // ALSO, the other factors to change if instability occurs is the collider's
                    // manifold epsilon, and its inertia tensor. A larger epsilon or larger tensor
                    // values will increase stability
                    
                    // "slop" damping
                    //const float flatDamping = 0.01f;

                    // linear percentage damping
                    //const float percentDamping = 0.98f;

                    // exponential damping which is stronger approaching 0 relative velocity at collision point
                    //const float invDampingStrength = 32;
                    //const float dynamicDamping = calculate_damping(relVelocity, invDampingStrength);

                    // exponential damping relative to difference of angular velocity
                    //const float relativeAV2 = glm::length2(thisPhysics.angularVelocity - otherPhysics.angularVelocity);
                    //const float avDamping = -1.0f / (1.0f + relativeAV2 * invDampingStrength) + 1.0f;


                    // STEP 7: dynamic resolution
                    // STEP 7.1: resolve linear normal impulse response
                    thisPhysics.velocity  += thisInvMass * (contactImpulse*collisionNormal);
                    otherPhysics.velocity -= otherInvMass * (contactImpulse*collisionNormal);

                    // STEP 7.2 resolve linear friction impulse response
                    thisPhysics.velocity  += thisInvMass * (frictionImpulse*collisionTangent);
                    otherPhysics.velocity -= otherInvMass * (frictionImpulse*collisionTangent);

                    // STEP 7.3: resolve angular normal impulse response
                    const glm::vec3 thisAngularNormalImpulse  = thisInvMoment * glm::cross(thisLever, (contactImpulse*collisionNormal));
                    const glm::vec3 otherAngularNormalImpulse = otherInvMoment * glm::cross(otherLever, (contactImpulse*collisionNormal));
                    thisPhysics.angularVelocity  += thisAngularNormalImpulse;
                    otherPhysics.angularVelocity -= otherAngularNormalImpulse;
                    
                    //PLEEPLOG_DEBUG("This Normal Angular Impulse: " + std::to_string(thisAngularNormalImpulse.x) + ", " + std::to_string(thisAngularNormalImpulse.y) + ", " + std::to_string(thisAngularNormalImpulse.z));
                    //PLEEPLOG_DEBUG("Other Normal Angular Impulse: " + std::to_string(-otherAngularNormalImpulse.x) + ", " + std::to_string(-otherAngularNormalImpulse.y) + ", " + std::to_string(-otherAngularNormalImpulse.z));
                    

                    // STEP 7.4 resolve angular friction impulse response
                    const glm::vec3 thisAngularFrictionImpulse  = thisInvMoment * glm::cross(thisLever, (frictionImpulse*collisionTangent));
                    const glm::vec3 otherAngularFrictionImpulse = otherInvMoment * glm::cross(otherLever, (frictionImpulse*collisionTangent));
                    thisPhysics.angularVelocity  += thisAngularFrictionImpulse;
                    otherPhysics.angularVelocity -= otherAngularFrictionImpulse;

                    // STEP 7.5: apply angular dampening
                    // we'll linearly damp angular velocities after impulse to try to break out of any equilibriums
                    thisPhysics.angularVelocity  *= 1.0f - thisPhysics.collisionAngularDrag;
                    otherPhysics.angularVelocity *= 1.0f - otherPhysics.collisionAngularDrag;
                    
                    //PLEEPLOG_DEBUG("This Friction Angular Impulse: " + std::to_string(thisAngularFrictionImpulse.x) + ", " + std::to_string(thisAngularFrictionImpulse.y) + ", " + std::to_string(thisAngularFrictionImpulse.z));
                    //PLEEPLOG_DEBUG("Other Friction Angular Impulse: " + std::to_string(-otherAngularFrictionImpulse.x) + ", " + std::to_string(-otherAngularFrictionImpulse.y) + ", " + std::to_string(-otherAngularFrictionImpulse.z));

                }
            }
        }

        // potentially useful exponential damping factor?
        static float calculate_damping(glm::vec3 vector, float invFactor)
        {
            return -1.0f / (1.0f +
                (vector.x*vector.x 
                + vector.y*vector.y
                + vector.z*vector.z) * invFactor) + 1.0f;
        }

        
        // store in a simple queue for now
        void submit(ColliderPacket data)
        {
            m_colliderPackets.push_back(data);
        }

        // clear packets for next frame
        void clear() override
        {
            m_colliderPackets.clear();
        }

    private:
        // TODO: hey dumb dumb figure out RTrees
        std::vector<ColliderPacket> m_colliderPackets;
    };
}

#endif // COLLISION_PHYSICS_RELAY_H