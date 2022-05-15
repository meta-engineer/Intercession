#include "spring_body_component.h"
#include "core/cosmos.h"
#include "physics/physics_component.h"
#include "physics/rigid_body_component.h"

namespace pleep
{
    void SpringBodyComponent::collision_response(IPhysicsResponseComponent* otherBody, ColliderPacket& thisData, ColliderPacket& otherData, glm::vec3& collisionNormal, float& collisionDepth, glm::vec3& collisionPoint)
    {
            // switch collisionNormal & collisionPoint
            glm::vec3 invCollisionNormal = -collisionNormal;
            glm::vec3 invCollisionPoint = collisionPoint - (collisionNormal * collisionDepth);
            otherBody->collision_response(this, otherData, thisData, invCollisionNormal, collisionDepth, invCollisionPoint);
    }

    void SpringBodyComponent::collision_response(SpringBodyComponent* otherSpringBody, ColliderPacket& thisData, ColliderPacket& otherData, glm::vec3& collisionNormal, float& collisionDepth, glm::vec3& collisionPoint)
    {
        // find spring force of each body, combine, then apply equal-and-opposite
        // apply friction

        PLEEPLOG_WARN("Spring-Spring response not implemented yet.");
        UNREFERENCED_PARAMETER(otherSpringBody);
        UNREFERENCED_PARAMETER(thisData);
        UNREFERENCED_PARAMETER(otherData);
        UNREFERENCED_PARAMETER(collisionNormal);
        UNREFERENCED_PARAMETER(collisionDepth);
        UNREFERENCED_PARAMETER(collisionPoint);
    }
    
    void SpringBodyComponent::collision_response(RigidBodyComponent* otherRigidBody, ColliderPacket& thisData, ColliderPacket& otherData, glm::vec3& collisionNormal, float& collisionDepth, glm::vec3& collisionPoint)
    {
        // find spring force of myself, then apply equal-and-opposite
        // apply friction
        
        // TODO: Make this exception safe! colliders don't necessarily have physics components
        PhysicsComponent& thisPhysics = thisData.owner->get_component<PhysicsComponent>(thisData.collidee);
        PhysicsComponent& otherPhysics = otherData.owner->get_component<PhysicsComponent>(otherData.collidee);
        
        // wake physics since collision has occurred? maybe this should be in top level dispatch
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
        // only use this's spring properties
        // TODO: centrialize this method for all collision_response()'s
        // max energy lost along tangent
        const float staticFrictionFactor  = this->staticFriction * otherRigidBody->staticFriction;
        // energy lost along tangent
        const float dynamicFrictionFactor = this->dynamicFriction * otherRigidBody->dynamicFriction;

        // STEP 2: Geometry Properties
        // STEP 2.1: center of mass
        const glm::vec3 thisCenterOfMass = thisData.transform.origin;
        const glm::vec3 otherCenterOfMass = otherData.transform.origin;
        
        // STEP 2.2: Collision Point Rationalizing
        // Because there is no static resolution, set collision point to midway point
        collisionPoint = collisionPoint - (collisionNormal * collisionDepth)/2.0f;

        // STEP 3.2: vector describing the "radius" of the rotation
        const glm::vec3 thisLever = (collisionPoint - thisCenterOfMass);
        const glm::vec3 otherLever = (collisionPoint - otherCenterOfMass);
        
        // STEP 3.3 relative velocity vector
        // relative is: this' velocity as viewed by other
        const glm::vec3 relVelocity = ((thisPhysics.velocity + glm::cross(thisPhysics.angularVelocity, thisLever)) - (otherPhysics.velocity + glm::cross(otherPhysics.angularVelocity, otherLever)));

        // NO early exit on negative relVelocity because springs will penetrate
        
        // STEP 3.4: angular inertia/moment
        // TODO: can this be optimized? inverse of inverse :(
        // TODO: moment doesn't behave correct with scaled transforms
        //   copy transforms, extract scale, build inertia tensor with scale
        //   then transform tensor with scale-less model transform
        // each collider can restrict it as they see fit
        const glm::mat3 thisInverseModel = glm::inverse(glm::mat3(thisData.collider->compose_transform(thisData.transform)));
        const glm::mat3 thisInvMoment = thisInvMass == 0 ? glm::mat3(0.0f) 
            : glm::inverse(
                glm::transpose(thisInverseModel) 
                * (thisData.collider->get_inertia_tensor(thisData.transform.scale) * thisPhysics.mass)
                * thisInverseModel
            );

        const glm::mat3 otherInverseModel = glm::inverse(glm::mat3(otherData.collider->compose_transform(otherData.transform)));
        const glm::mat3 otherInvMoment = otherInvMass == 0 ? glm::mat3(0.0f)
            : glm::inverse(
                glm::transpose(otherInverseModel)
                * (otherData.collider->get_inertia_tensor(otherData.transform.scale) * otherPhysics.mass)
                * otherInverseModel
            );

        // STEP 3.5: Spring properties
        // spring length = collisionDepth
        // delta spring length is relativeVelocity along collision Normal
        const glm::vec3 deltaCollisionDepth = glm::dot(relVelocity, collisionNormal) * collisionNormal;
        // if dot product is posative -> this is movign away from other
        // therefore spring is changing posatively

        // STEP 4: Spring Force
        // spring body may have a preference for direction
        // for now we can apply along surface normal
        const float springForceMagnitude = (collisionDepth - this->restLength) * this->stiffness;
        const glm::vec3 springForce = springForceMagnitude * collisionNormal;
        const glm::vec3 dampedSpringForce = springForce - this->damping * deltaCollisionDepth;

        // STEP 5: Friction
        // STEP 5.1: Determine velocity perpendicular to normal (tangent along surface)
        const glm::vec3 tangentCross = glm::cross(relVelocity, collisionNormal);
        glm::vec3 collisionTangent = glm::vec3(0.0f);
        if (glm::length2(tangentCross) != 0.0f)
        {
            // cross normal with tangent cross to get tangent direction
            collisionTangent = glm::cross(collisionNormal, tangentCross);
            //PLEEPLOG_DEBUG("Collision Tangent pre-normalize: " + std::to_string(collisionTangent.x) + ", " + std::to_string(collisionTangent.y) + ", " + std::to_string(collisionTangent.z));
            if (glm::length2(collisionTangent) != 0.0f)
                collisionTangent = glm::normalize(collisionTangent);
        }
        
        // STEP 5.2: Determine friction impulse
        const float tangentImpulse = -1.0f * glm::dot(relVelocity, collisionTangent) /
            (otherInvMass + thisInvMass +
            glm::dot(
                glm::cross(otherInvMoment * glm::cross(otherLever, collisionTangent), otherLever) + 
                glm::cross(thisInvMoment * glm::cross(thisLever, collisionTangent), thisLever), 
                collisionTangent)
            );
            
        // STEP 5.3: Coefficient factors
        // if impulse is less than static max, then apply it (this should negate all colinear velocity)
        // if impulse is greater than static max, multiply it by dynamic coefficient
        const float frictionCone = staticFrictionFactor * springForceMagnitude;
        //PLEEPLOG_DEBUG("Static friction limit: " + std::to_string(frictionCone));

        const float frictionImpulse = std::abs(tangentImpulse) < std::abs(frictionCone) ? tangentImpulse : tangentImpulse * dynamicFrictionFactor;

        // STEP 6: Dynamic resolution
        // STEP 6.1: resolve linear spring force
        thisPhysics.acceleration  +=  thisInvMass * dampedSpringForce;
        otherPhysics.acceleration -= otherInvMass * dampedSpringForce;

        // STEP 6.2 resolve linear friction impulse response
        thisPhysics.velocity  += thisInvMass * (frictionImpulse*collisionTangent);
        otherPhysics.velocity -= otherInvMass * (frictionImpulse*collisionTangent);

        // STEP 6.3 resolve angular spring force
        if (this->m_influenceOrientation)
        {
            thisPhysics.angularAcceleration  += thisInvMoment * glm::cross(thisLever, dampedSpringForce);
        }
        if (otherRigidBody->m_influenceOrientation)
        {
            otherPhysics.angularAcceleration -= otherInvMoment * glm::cross(otherLever, dampedSpringForce);
        }

        // STEP 6.4 resolve angular friction impulse response
        if (this->m_influenceOrientation)
        {
            thisPhysics.angularVelocity  += thisInvMoment * glm::cross(thisLever, frictionImpulse*collisionTangent);
        }
        if (otherRigidBody->m_influenceOrientation)
        {
            otherPhysics.angularVelocity -= otherInvMoment * glm::cross(otherLever, frictionImpulse*collisionTangent);
        }

        // STEP 6.5: apply angular dampening
        // we'll linearly damp angular velocities after impulse to try to break out of any equilibriums
        if (this->m_influenceOrientation)
        {
            thisPhysics.angularVelocity  *= 1.0f - thisPhysics.collisionAngularDrag;
        }
        if (otherRigidBody->m_influenceOrientation)
        {
            otherPhysics.angularVelocity *= 1.0f - otherPhysics.collisionAngularDrag;
        }
    }
}