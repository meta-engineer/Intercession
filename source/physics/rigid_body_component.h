#ifndef RIGID_BODY_COMPONENT_H
#define RIGID_BODY_COMPONENT_H

//#include "intercession_pch.h"
#include "physics/i_physics_response_component.h"

namespace pleep
{
    // define attributes needed for rigidbody physics response
    struct RigidBodyComponent : public IPhysicsResponseComponent
    {
        // proportion of energy *retained* normal response
        float restitution       = 0.50f;
        // proportion of energy *lost* through friction
        float staticFriction    = 0.20f;
        float dynamicFriction   = 0.20f;

        // ***** Collision Response methods *****
        // double dispatch other
        void collision_response(IPhysicsResponseComponent* otherBody, ColliderPacket& thisData, ColliderPacket& otherData, glm::vec3& collisionNormal, float& collisionDepth, glm::vec3& collisionPoint) override
        {
            // switch collisionNormal & collisionPoint
            glm::vec3 invCollisionNormal = -collisionNormal;
            glm::vec3 invCollisionPoint = collisionPoint - (collisionNormal * collisionDepth);
            otherBody->collision_response(this, otherData, thisData, invCollisionNormal, collisionDepth, invCollisionPoint);
        }
        
        void collision_response(RigidBodyComponent* otherRigidBody, ColliderPacket& thisData, ColliderPacket& otherData, glm::vec3& collisionNormal, float& collisionDepth, glm::vec3& collisionPoint)
        {
            // carry out collision between two Rigid bodies
            // TODO: Make this exception safe! colliders without physics components will throw!
            PhysicsComponent& thisPhysics = thisData.owner->get_component<PhysicsComponent>(thisData.collidee);
            PhysicsComponent& otherPhysics = otherData.owner->get_component<PhysicsComponent>(otherData.collidee);

            // wake physics since collision has occurred?
            //thisPhysics.isAsleep = false;
            //otherPhysics.isAsleep = false;

            // STEP 1: Fetch entity properties
            // STEP 1.1: material physics properties
            // check mutability of each object (& test for early exit)
            if (thisPhysics.mass == INFINITE_MASS && otherPhysics.mass == INFINITE_MASS)
                return;
            
            // calculate inverse mass for convenience (account for Infinite mass)
            float thisInvMass = 0;
            if (thisPhysics.mass != INFINITE_MASS)
            {
                thisInvMass = 1.0f/thisPhysics.mass;
            }
            //PLEEPLOG_DEBUG("this inverse mass: " + std::to_string(thisInvMass));
            float otherInvMass = 0;
            if (otherPhysics.mass != INFINITE_MASS)
            {
                otherInvMass = 1.0f/otherPhysics.mass;
            }
            //PLEEPLOG_DEBUG("other inverse mass: " + std::to_string(otherInvMass));
            
            // STEP 1.2: material collision properties
            // TODO: average or product of material properties?
            // energy retained along normal
            const float restitutionFactor     = (this->restitution + otherRigidBody->restitution) / 2.0f;
            // max energy lost along tangent
            const float staticFrictionFactor  = (this->staticFriction + otherRigidBody->staticFriction) / 2.0f;
            // energy lost along tangent
            const float dynamicFrictionFactor = (this->dynamicFriction + otherRigidBody->dynamicFriction) / 2.0f;
            
            // STEP 2: static resolution
            // TODO: static resolution can cause objects in complex scenarios to "walk around" and not
            //   abide by the stability of the dynamic resolution, this can only be fixed with
            //   a continuous intersect detection and continuous resolution, so its a major refactor

            // STEP 2.1: resolve transform origin and collision point (on other)
            // share static resolution based on relative mass proportion
            // we are guarenteed to have at least one not be INFINITE_MASS 
            // 1 -> move only this, 0 -> move only other
            float massRatio = thisInvMass/(thisInvMass + otherInvMass);
            //PLEEPLOG_DEBUG("MassRatio of this: " + std::to_string(massRatio));

            // collisionNormal is in direction away from "other", towards "this"
            thisData.transform.origin  += collisionNormal * collisionDepth * massRatio;
            otherData.transform.origin -= collisionNormal * collisionDepth * (1-massRatio);
            collisionPoint             -= collisionNormal * collisionDepth * (1-massRatio);

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

            // STEP 3.2 relative velocity vector
            // relative is: this' velocity as viewed by other
            const glm::vec3 relVelocity = ((thisPhysics.velocity + glm::cross(thisPhysics.angularVelocity, thisLever)) - (otherPhysics.velocity + glm::cross(otherPhysics.angularVelocity, otherLever)));
            //PLEEPLOG_DEBUG("Relative Velocity at collision: " + std::to_string(relVelocity.x) + ", " + std::to_string(relVelocity.y) + ", " + std::to_string(relVelocity.z));

            // early exit if colliders are already moving away from eachother at collisionPoint
            if (glm::dot(relVelocity, collisionNormal) > 0)
            {
                //PLEEPLOG_DEBUG("Colliding rigid bodies are already moving away from one another, so I won't interupt");
                return;
            }

            // STEP 3.3: angular inertia/moment
            // TODO: can this be optimized? inverse of inverse :(
            // TODO: moment doesn't behave correct with scaled transforms
            //   (and maybe also collider offset transforms)
            const glm::mat3 thisInverseModel = glm::inverse(glm::mat3(thisData.transform.get_model_transform() * thisData.collider->m_localTransform.get_model_transform()));
            const glm::mat3 thisInvMoment = thisInvMass == 0 ? glm::mat3(0.0f) 
                : glm::inverse(
                    glm::transpose(thisInverseModel) 
                    * (thisData.collider->get_inertia_tensor() * thisPhysics.mass)
                    * thisInverseModel
                );

            const glm::mat3 otherInverseModel = glm::inverse(glm::mat3(otherData.transform.get_model_transform() * otherData.collider->m_localTransform.get_model_transform()));
            const glm::mat3 otherInvMoment = otherInvMass == 0 ? glm::mat3(0.0f)
                : glm::inverse(
                    glm::transpose(otherInverseModel)
                    * (otherData.collider->get_inertia_tensor() * otherPhysics.mass)
                    * otherInverseModel
                );


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
            const float frictionCone = (1.0f - staticFrictionFactor) * contactImpulse;
            //PLEEPLOG_DEBUG("Static friction limit: " + std::to_string(frictionCone));

            const float frictionImpulse = std::abs(tangentImpulse) < std::abs(frictionCone) ? tangentImpulse : tangentImpulse * (1.0f - dynamicFrictionFactor);
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
    };
}

#endif // RIGID_BODY_COMPONENT_H