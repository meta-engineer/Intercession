#ifndef COLLISION_PHYSICS_RELAY_H
#define COLLISION_PHYSICS_RELAY_H

//#include "intercession_pch.h"
#include <vector>

#include "logging/pleep_log.h"
#include "physics/a_physics_relay.h"
#include "physics/collider_packet.h"
#include "core/cosmos.h"
#include "behaviors/behaviors_component.h"
#include "physics/rigid_body_component.h"
#include "physics/spring_body_component.h"

namespace pleep
{
    class CollisionPhysicsRelay : public A_PhysicsRelay
    {
    public:
        // explicitly inherit constructors
        using A_PhysicsRelay::A_PhysicsRelay;

        // motion integration should already have happened
        // test for collision, do static resolution, do dynamic resolution
        void engage(double deltaTime) override
        {
            // unused for discrete collision detection
            UNREFERENCED_PARAMETER(deltaTime);

            for (std::vector<ColliderPacket>::iterator thisPacket_it = m_colliderPackets.begin(); thisPacket_it != m_colliderPackets.end(); thisPacket_it++)
            {
                ColliderPacket& thisData = *thisPacket_it;
                if (thisData.collider->get_type() == ColliderType::none || !thisData.collider->isActive)
                    continue;

                // no spacial partitioning :(
                // TODO: If collider can only collide once (like ray) we have to track the pair which maximizes the collider's criteria (closeness) and only invoke response between those
                for (std::vector<ColliderPacket>::iterator otherPacket_it = thisPacket_it + 1; otherPacket_it != m_colliderPackets.end(); otherPacket_it++)
                {
                    assert(otherPacket_it != thisPacket_it);
                    ColliderPacket& otherData = *otherPacket_it;

                    std::shared_ptr<Cosmos> cosmos = thisData.owner.lock();
                    assert(!thisData.owner.expired());
                    assert(cosmos == otherData.owner.lock());

                    // check other collider type (for removing double-dispatch later)
                    if (otherData.collider->get_type() == ColliderType::none || !otherData.collider->isActive)
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

                    // TODO: Check if entities have any behaviors/physics responses BEFORE intersect check and exit early!

                    // ***** PHYSICS RESPONSE *****
                    // Forward collision data to be resolved according to the response component
                    I_PhysicsResponseComponent* thisResponse = nullptr;
                    I_PhysicsResponseComponent* otherResponse = nullptr;

                    try
                    {
                        switch(thisData.collider->responseType)
                        {
                            case CollisionResponseType::rigid:
                                thisResponse = &(cosmos->get_component<RigidBodyComponent>(thisData.collidee));
                                break;
                            case CollisionResponseType::spring:
                                thisResponse = &(cosmos->get_component<SpringBodyComponent>(thisData.collidee));
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
                        PLEEPLOG_WARN("Could not find physics response component for collider's set CollisionResponseType " + std::to_string(thisData.collider->responseType) + ", clearing and skipping");
                        thisData.collider->responseType = CollisionResponseType::noop;
                    }
                    try
                    {
                        switch(otherData.collider->responseType)
                        {
                            case CollisionResponseType::rigid:
                                otherResponse = &(cosmos->get_component<RigidBodyComponent>(otherData.collidee));
                                break;
                            case CollisionResponseType::spring:
                                otherResponse = &(cosmos->get_component<SpringBodyComponent>(otherData.collidee));
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
                        PLEEPLOG_WARN("Could not find physics response component for collider's set CollisionResponseType " + std::to_string(otherData.collider->responseType) + ", clearing and skipping");
                        otherData.collider->responseType = CollisionResponseType::noop;
                    }

                    // Check for un-handled CollisionResponseType
                    if (thisResponse != nullptr && otherResponse != nullptr)
                    {
                        // Again, virtual dispatch may not be very optimal, but it is easy to understand
                        thisResponse->collision_response(otherResponse, thisData, otherData, collisionNormal, collisionDepth, collisionPoint);
                    }

                    // ***** BEHAVIORS RESPONSE *****
                    // behaviors method should be called just AFTER the physics response (static/dynamic resolution)
                    // CAREFUL! BehaviorsComponent fetch could fail OR behaviors drivetrain could be null
                    // TODO: can collision relay track when a collision enter/exit across frames? Which entities collided last frame?
                    // TODO: is it inefficient to need to search for the behaviors each time?
                    if (thisData.collider->useBehaviorsResponse == true)
                    {
                        try
                        {
                            BehaviorsComponent& behaviors = cosmos->get_component<BehaviorsComponent>(thisData.collidee);
                            if (!behaviors.drivetrain)
                            {
                                throw std::runtime_error("Cannot call collision behavior response for null BehaviorsDrivetrain");
                            }
                            behaviors.drivetrain->on_collision(thisData, otherData, collisionNormal, collisionDepth, collisionPoint, m_sharedBroker);
                        }
                        catch(const std::exception& err)
                        {
                            UNREFERENCED_PARAMETER(err);
                            //PLEEPLOG_WARN(err.what());
                            PLEEPLOG_WARN("Collidee entity (" + std::to_string(thisData.collidee) + ") could not trigger behavior response, disabling and skipping");
                            thisData.collider->useBehaviorsResponse = false;
                        }
                    }
                    if (otherData.collider->useBehaviorsResponse == true)
                    {
                        try
                        {
                            // invert relative collision metadata
                            glm::vec3 invCollisionNormal = -collisionNormal;
                            glm::vec3 invCollisionPoint = collisionPoint - (collisionNormal * collisionDepth);
                            
                            BehaviorsComponent& behaviors = cosmos->get_component<BehaviorsComponent>(otherData.collidee);
                            if (!behaviors.drivetrain)
                            {
                                throw std::runtime_error("Cannot call collision behavior response for null BehaviorsDrivetrain");
                            }
                            behaviors.drivetrain->on_collision(otherData, thisData, invCollisionNormal, collisionDepth, invCollisionPoint, m_sharedBroker);
                        }
                        catch(const std::exception& err)
                        {
                            UNREFERENCED_PARAMETER(err);
                            //PLEEPLOG_WARN(err.what());
                            PLEEPLOG_WARN("Collidee entity (" + std::to_string(otherData.collidee) + ") could not trigger behavior response, disabling and skipping");
                            otherData.collider->useBehaviorsResponse = false;
                        }
                    }

                    // check causal chain link descrepancy
                    CausalChainlink thisLink  = derive_causal_chain_link(thisData.collidee);
                    CausalChainlink otherLink = derive_causal_chain_link(otherData.collidee);

                    // check timestream state descrepancy
                    const TimestreamState thisState = cosmos->get_timestream_state(thisData.collidee).first;
                    const TimestreamState otherState = cosmos->get_timestream_state(otherData.collidee).first;

                    // Interception can only happen if
                    //      neither entity has null chainlink
                    //  AND either
                    //      one entity has chainlink 0 AND the other is not chainlink 0
                    //  OR
                    //      neither entity is chainlink 0 and one entity has a "forked" state AND the other is not forked
                    if ((thisLink  != NULL_CAUSALCHAINLINK) && (otherLink != NULL_CAUSALCHAINLINK) && (
                            ((thisLink==0) ^ (otherLink==0))
                            ||
                            (thisLink!=0 && otherLink!=0 && (is_divergent(thisState) ^ is_divergent(otherState)))
                        )
                       )
                    {
                        // Signal to NetworkDynamo put this entity's future into superposition
                        // (and propagate up the timeline from there)
                        EventMessage interceptionMessage(events::cosmos::TIMESTREAM_INTERCEPTION);
                        // select the 0 link or forked state entity as the "agent"
                        events::cosmos::TIMESTREAM_INTERCEPTION_params interceptionInfo {
                            thisLink==0 || otherLink!=0 && is_divergent(thisState) ? thisData.collidee :  otherData.collidee
                        };
                        interceptionInfo.recipient = interceptionInfo.agent == thisData.collidee ? otherData.collidee : thisData.collidee;
                        interceptionMessage << interceptionInfo;
                        m_sharedBroker->send_event(interceptionMessage);
                    }
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
        // TODO: hey dumb-dumb figure out RTrees
        std::vector<ColliderPacket> m_colliderPackets;
    };
}

#endif // COLLISION_PHYSICS_RELAY_H