#ifndef COLLISION_PHYSICS_RELAY_H
#define COLLISION_PHYSICS_RELAY_H

//#include "intercession_pch.h"
#include <vector>

#include "logging/pleep_log.h"
#include "physics/i_physics_relay.h"
#include "physics/collider_packet.h"
#include "core/cosmos.h"
#include "physics/rigid_body_component.h"
#include "physics/spring_body_component.h"

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
                    assert(thisData.owner == otherData.owner);

                    // check other collider type (for removing double-dispatch later)
                    if (otherData.collider->get_type() == ColliderType::none)
                        continue;

                    // check for colliders of same entity
                    // TODO: check for colliders in same entity heirarchy
                    if (thisData.collidee == otherData.collidee)
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

                    // TODO: Call script collision response
                    // TODO: Check if entities have script/physics response BEFORE intersect check!

                    // Forward collision data to be resolved according to the physics response
                    IPhysicsResponseComponent* thisResponse = nullptr;
                    IPhysicsResponseComponent* otherResponse = nullptr;

                    try
                    {
                        switch(thisData.collider->m_responseType)
                        {
                            case CollisionResponseType::rigid:
                                thisResponse = &(thisData.owner->get_component<RigidBodyComponent>(thisData.collidee));
                                break;
                            case CollisionResponseType::spring:
                                thisResponse = &(thisData.owner->get_component<SpringBodyComponent>(thisData.collidee));
                                break;
                            default:
                                // leave as nullptr
                                break;
                        }
                    }
                    catch(const std::exception& err)
                    {
                        UNREFERENCED_PARAMETER(err);
                        PLEEPLOG_WARN(err.what());
                        PLEEPLOG_WARN("Could not find physics response component for collider's set CollisionResponseType " + std::to_string(thisData.collider->m_responseType) + ", clearing and skipping");
                        thisData.collider->m_responseType = CollisionResponseType::none;
                    }
                    try
                    {
                        switch(otherData.collider->m_responseType)
                        {
                            case CollisionResponseType::rigid:
                                otherResponse = &(otherData.owner->get_component<RigidBodyComponent>(otherData.collidee));
                                break;
                            case CollisionResponseType::spring:
                                otherResponse = &(otherData.owner->get_component<SpringBodyComponent>(otherData.collidee));
                                break;
                            default:
                                // leave as nullptr
                                break;
                        }
                    }
                    catch(const std::exception& err)
                    {
                        UNREFERENCED_PARAMETER(err);
                        PLEEPLOG_WARN(err.what());
                        PLEEPLOG_WARN("Could not find physics response component for collider's set CollisionResponseType " + std::to_string(otherData.collider->m_responseType) + ", clearing and skipping");
                        otherData.collider->m_responseType = CollisionResponseType::none;
                    }

                    if (thisResponse == nullptr || otherResponse == nullptr)
                    {
                        // one of the colliders is non-physical
                        continue;
                    }

                    // Again, virtual dispatch may not be very optimal, but it is easy to understand
                    thisResponse->collision_response(otherResponse, thisData, otherData, collisionNormal, collisionDepth, collisionPoint);
                }
            }
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