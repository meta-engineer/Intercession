#ifndef COLLISION_PHYSICS_RELAY_H
#define COLLISION_PHYSICS_RELAY_H

//#include "intercession_pch.h"
#include <vector>

#include "logging/pleep_log.h"
#include "physics/a_physics_relay.h"
#include "physics/collider_packet.h"
#include "core/cosmos.h"
#include "behaviors/behaviors_component.h"
#include "physics/collision_procedures.h"

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

            for (std::vector<ColliderPacket>::iterator packetA_it = m_colliderPackets.begin(); packetA_it != m_colliderPackets.end(); packetA_it++)
            {
                ColliderPacket& dataA = *packetA_it;
                if (dataA.collider.colliderType == ColliderType::none || !dataA.collider.isActive)
                    continue;

                // no spacial partitioning :(
                // TODO: If collider can only collide once (like ray) we have to track the pair which maximizes the collider's criteria (closeness) and only invoke response between those
                for (std::vector<ColliderPacket>::iterator packetB_it = packetA_it + 1; packetB_it != m_colliderPackets.end(); packetB_it++)
                {
                    assert(packetB_it != packetA_it);
                    ColliderPacket& dataB = *packetB_it;

                    std::shared_ptr<Cosmos> cosmos = dataA.owner.lock();
                    assert(!dataA.owner.expired());
                    assert(cosmos == dataB.owner.lock());

                    // check other collider type (for removing double-dispatch later)
                    if (dataB.collider.colliderType == ColliderType::none || !dataB.collider.isActive)
                        continue;

                    // check for colliders of same entity
                    if (dataA.collidee == dataB.collidee)
                        continue;

                    // ***** COLLISION INTERSECTION CHECK *****
                    // Get collision data
                    glm::vec3 collisionNormal;
                    float collisionDepth;
                    glm::vec3 collisionPoint;

                    // Lookup intersection function and call with data for A & B
                    bool hit = intersectProcedures[static_cast<size_t>(dataA.collider.colliderType)]
                                                  [static_cast<size_t>(dataB.collider.colliderType)](
                        dataA, dataB, collisionPoint, collisionNormal, collisionDepth
                    );
                    if (!hit)
                    {
                        continue;
                    }
                    //PLEEPLOG_DEBUG("Collision Detected!");
                    //PLEEPLOG_DEBUG("Collision Point: " + std::to_string(collisionPoint.x) + ", " + std::to_string(collisionPoint.y) + ", " + std::to_string(collisionPoint.z));
                    //PLEEPLOG_DEBUG("Collision Normal: " + std::to_string(collisionNormal.x) + ", " + std::to_string(collisionNormal.y) + ", " + std::to_string(collisionNormal.z));
                    //PLEEPLOG_DEBUG("Collision Depth: " + std::to_string(collisionDepth));
                    //PLEEPLOG_DEBUG("A @: " + std::to_string(dataA.transform.origin.x) + ", " + std::to_string(dataA.transform.origin.y) + ", " + std::to_string(dataA.transform.origin.z));
                    //PLEEPLOG_DEBUG("B @: " + std::to_string(dataB.transform.origin.x) + ", " + std::to_string(dataB.transform.origin.y) + ", " + std::to_string(dataB.transform.origin.z));


                    // TODO: Check if entities have any behaviors/physics responses BEFORE intersect check and exit early!

                    // ***** PHYSICS RESPONSE *****
                    // Ensure entities with a physics response have physics components
                    
                    if (dataA.collider.collisionType != CollisionType::noop && dataB.collider.collisionType != CollisionType::noop)
                    {
                        try
                        {
                            PhysicsComponent& physicsA = dataA.owner.lock()->get_component<PhysicsComponent>(dataA.collidee);
                            PhysicsComponent& physicsB = dataB.owner.lock()->get_component<PhysicsComponent>(dataB.collidee);

                            // Lookup response function and call with data for A & B
                            responseProcedures[static_cast<size_t>(dataA.collider.collisionType)]
                                              [static_cast<size_t>(dataB.collider.collisionType)](
                                dataA, physicsA,
                                dataB, physicsB,
                                collisionPoint, collisionNormal, collisionDepth
                            );

                        }
                        catch (const std::runtime_error& err)
                        {
                            UNREFERENCED_PARAMETER(err);
                            // PhysicsComponent TYPE does not exist in cosmos
                            // or
                            // PhysicsComponent for an entity does not exist in cosmos
                            // could set its response type to noop?

                        }
                    }

                    // ***** BEHAVIORS RESPONSE *****
                    // behaviors method should be called just AFTER the physics response (static/dynamic resolution)
                    // CAREFUL! BehaviorsComponent fetch could fail OR behaviors drivetrain could be null
                    // TODO: can collision relay track when a collision enter/exit across frames? Which entities collided last frame?
                    // TODO: is it inefficient to need to search for the behaviors each time?
                    if (dataA.collider.useBehaviorsResponse == true)
                    {
                        try
                        {
                            BehaviorsComponent& behaviors = cosmos->get_component<BehaviorsComponent>(dataA.collidee);
                            if (!behaviors.drivetrain)
                            {
                                throw std::runtime_error("Cannot call collision behavior response for null BehaviorsDrivetrain");
                            }
                            behaviors.drivetrain->on_collision(dataA, dataB, collisionNormal, collisionDepth, collisionPoint, m_sharedBroker);
                        }
                        catch(const std::exception& err)
                        {
                            UNREFERENCED_PARAMETER(err);
                            //PLEEPLOG_WARN(err.what());
                            PLEEPLOG_WARN("Collidee entity (" + std::to_string(dataA.collidee) + ") could not trigger behavior response, disabling and skipping");
                            dataA.collider.useBehaviorsResponse = false;
                        }
                    }
                    if (dataB.collider.useBehaviorsResponse == true)
                    {
                        try
                        {
                            // invert relative collision metadata
                            glm::vec3 invCollisionNormal = -collisionNormal;
                            glm::vec3 invCollisionPoint = collisionPoint - (collisionNormal * collisionDepth);
                            
                            BehaviorsComponent& behaviors = cosmos->get_component<BehaviorsComponent>(dataB.collidee);
                            if (!behaviors.drivetrain)
                            {
                                throw std::runtime_error("Cannot call collision behavior response for null BehaviorsDrivetrain");
                            }
                            behaviors.drivetrain->on_collision(dataB, dataA, invCollisionNormal, collisionDepth, invCollisionPoint, m_sharedBroker);
                        }
                        catch(const std::exception& err)
                        {
                            UNREFERENCED_PARAMETER(err);
                            //PLEEPLOG_WARN(err.what());
                            PLEEPLOG_WARN("Collidee entity (" + std::to_string(dataB.collidee) + ") could not trigger behavior response, disabling and skipping");
                            dataB.collider.useBehaviorsResponse = false;
                        }
                    }

                    // ***** TIMESTREAM RESPONSE *****
                    // check causal chain link descrepancy
                    CausalChainlink thisLink  = derive_causal_chain_link(dataA.collidee);
                    CausalChainlink otherLink = derive_causal_chain_link(dataB.collidee);

                    // check timestream state descrepancy
                    const TimestreamState thisState = cosmos->get_timestream_state(dataA.collidee).first;
                    const TimestreamState otherState = cosmos->get_timestream_state(dataB.collidee).first;

                    // Interception can only happen if chainlink values are different
                    // OR (chainlink values are the same but,) ONE entity is forked (other is not)

                    // we want link-0 entities to continuously trigger interceptions when affecting non-link-0 entities (because they can be non-deterministically changing) 
                    // we want non-link-0 entites to trigger interceptions only once when affecting higher-link entities (both parties becoming forked)
                    // we want forked entities to trigger interceptions only once when affecting non-forked entities (turning the other one forked)

                    /*    
                                =0 & F  =0 & M  >0 & F  >0 & M
                        =0 & F    X       X       !       !
                        =0 & M    X       X       !       !
                        >0 & F    !       !       X       !
                        >0 & M    !       !       !       X* (! if not a server and link values are not equal)

                        (All entities become forked if interception occurs, except link-0 entities)
                        (agent priority is: link0 entity, if not, forked entity)
                    */

                    // Parallel cosmos' never have link-0 entities, so they will never receive that case

                    // Server cosmos` do not want to trigger divergences for entities following their timestream, so they should only consider cases with link0 or forked

                    // no timestream response if either are null chainlink (non-temporal)
                    if (thisLink == NULL_CAUSALCHAINLINK || otherLink == NULL_CAUSALCHAINLINK)
                    {
                        continue;
                    }

                    //PLEEPLOG_DEBUG("Potential interception between: " + std::to_string(dataA.collidee) + "(" + std::to_string(thisState) + ") and " + std::to_string(dataB.collidee) + "(" + std::to_string(thisState) + ")");

                    // easier to check negative case
                    if (!(
                        ((thisLink == 0) ^ (otherLink == 0))
                        || (thisLink != 0 && otherLink != 0 && is_divergent(thisState) ^ is_divergent(otherState))
                        || (thisLink != otherLink && (!is_divergent(thisState) || !is_divergent(otherState)) && cosmos->get_host_id() == NULL_TIMESLICEID)
                        ))
                    {
                        continue;
                    }

                    // Signal to NetworkDynamo put this entity's future into superposition
                    // (and propagate up the timeline from there)
                    EventMessage interceptionMessage(events::cosmos::TIMESTREAM_INTERCEPTION);
                    events::cosmos::TIMESTREAM_INTERCEPTION_params interceptionInfo;
                    // agent priority is link0 -> forked -> lower-link
                    if ((thisLink == 0) ^ (otherLink == 0))
                    {
                        interceptionInfo.agent = thisLink == 0 ? dataA.collidee : dataB.collidee;
                    }
                    else if (is_divergent(thisState) ^ is_divergent(otherState))
                    {
                        interceptionInfo.agent = is_divergent(thisState) ? dataA.collidee : dataB.collidee;
                    }
                    else if (thisLink != otherLink)
                    {
                        interceptionInfo.agent = thisLink < otherLink ? dataA.collidee : dataB.collidee;
                    }
                    else
                    {
                        PLEEPLOG_ERROR("You fucked up, not all cases were captured, this code should never run");
                        interceptionInfo.agent = NULL_ENTITY;
                        assert(true);
                    }
                    // set recipient to be reciprocal
                    interceptionInfo.recipient = interceptionInfo.agent == dataA.collidee ? dataB.collidee : dataA.collidee;
                    interceptionMessage << interceptionInfo;
                    m_sharedBroker->send_event(interceptionMessage);
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