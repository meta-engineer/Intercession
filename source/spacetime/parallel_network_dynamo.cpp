#include "parallel_network_dynamo.h"

#include "logging/pleep_log.h"
#include "ecs/ecs_types.h"

namespace pleep
{
    ParallelNetworkDynamo::ParallelNetworkDynamo(std::shared_ptr<EventBroker> sharedBroker, TimelineApi localTimelineApi) 
        : I_NetworkDynamo(sharedBroker)
        , m_timelineApi(localTimelineApi)
    {
        PLEEPLOG_TRACE("Start parallel networking pipeline setup");

        // handle entity created/removed/intercepted for extraction later?
        // Do not send ENTITY_CREATED/REMOVED events to entity's host timeslice

        m_sharedBroker->add_listener(METHOD_LISTENER(events::cosmos::TIMESTREAM_INTERCEPTION, ParallelNetworkDynamo::_timestream_interception_handler));
        m_sharedBroker->add_listener(METHOD_LISTENER(events::parallel::INIT, ParallelNetworkDynamo::_parallel_init_handler));
        m_sharedBroker->add_listener(METHOD_LISTENER(events::parallel::FINISHED, ParallelNetworkDynamo::_parallel_finished_handler));
        
        PLEEPLOG_TRACE("Done parallel networking pipeline setup");
    }
    
    ParallelNetworkDynamo::~ParallelNetworkDynamo() 
    {
        m_sharedBroker->remove_listener(METHOD_LISTENER(events::cosmos::TIMESTREAM_INTERCEPTION, ParallelNetworkDynamo::_timestream_interception_handler));
        m_sharedBroker->remove_listener(METHOD_LISTENER(events::parallel::INIT, ParallelNetworkDynamo::_parallel_init_handler));
        m_sharedBroker->remove_listener(METHOD_LISTENER(events::parallel::FINISHED, ParallelNetworkDynamo::_parallel_finished_handler));
    }
    
    void ParallelNetworkDynamo::run_relays(double deltaTime) 
    {
        UNREFERENCED_PARAMETER(deltaTime);
        
        std::shared_ptr<Cosmos> cosmos = m_workingCosmos.lock();
        if (m_workingCosmos.expired()) cosmos = nullptr;
        
        uint16_t currentCoherency = 0;
        if (cosmos) currentCoherency = cosmos->get_coherency();
        
        /// handle incoming timestream messages from linked future timeslice
        //_process_timestream_messages();
        if (m_timelineApi.has_future())
        {
            std::vector<Entity> availableEntities = m_timelineApi.get_entities_with_future_streams();

            for (Entity& entity : availableEntities)
            {
                EventMessage evnt(1);
                // timeline api will return false if nothing available at currentCoherency or earlier
                while (m_timelineApi.pop_future_timestream(entity, currentCoherency, evnt))
                {
                    // continue to clear out messages if no working cosmos
                    if (!cosmos) continue;

                    switch(evnt.header.id)
                    {
                    case events::cosmos::ENTITY_UPDATE:
                    {
                        // ONLY EXTRACT UPSTREAM COMPONENTS!
                        events::cosmos::ENTITY_UPDATE_params updateInfo;
                        evnt >> updateInfo;

                        if (!cosmos->entity_exists(updateInfo.entity))
                        {
                            break;
                        }

                        Signature upstreamSign = cosmos->get_category_signature(ComponentCategory::upstream);

                        // extract and dump any components I don't want
                        // without having to know each one specifically
                        for (ComponentType i = 0; i < MAX_COMPONENT_TYPES; i++)
                        {
                            if (updateInfo.sign.test(i))
                            {
                                if (upstreamSign.test(i))
                                {
                                    cosmos->deserialize_single_component(updateInfo.entity, i, evnt);
                                }
                                else
                                {
                                    cosmos->discard_single_component(i, evnt);
                                }
                            }
                        }

                    }
                    break;
                    case events::cosmos::ENTITY_CREATED:
                    {
                        // this could be an entity created after jumping
                        // same as server?
                        events::cosmos::ENTITY_CREATED_params createInfo;
                        evnt >> createInfo;
                        PLEEPLOG_DEBUG("Create Entity: " + std::to_string(createInfo.entity) + " | " + createInfo.sign.to_string());

                        if (cosmos->register_entity(createInfo.entity))
                        {
                            for (ComponentType c = 0; c < createInfo.sign.size(); c++)
                            {
                                if (createInfo.sign.test(c)) cosmos->add_component(createInfo.entity, c);
                            }
                        }
                    }
                    break;
                    case events::cosmos::ENTITY_REMOVED:
                    {
                        // repeat removal
                        events::cosmos::ENTITY_REMOVED_params removeInfo;
                        evnt >> removeInfo;
                        PLEEPLOG_TRACE("Remove Entity: " + std::to_string(removeInfo.entity));

                        // use condemn event to avoid double deletion
                        cosmos->condemn_entity(removeInfo.entity);
                    }
                    break;
                    default:
                    {
                        PLEEPLOG_DEBUG("Parallel encountered unknown timestream event: " + std::to_string(evnt.header.id));
                    }
                    }
                }
            }
        }
    }
    
    void ParallelNetworkDynamo::reset_relays() 
    {
        m_workingCosmos.reset();

        // any relays?
    }
    
    void ParallelNetworkDynamo::submit(CosmosAccessPacket data) 
    {
        m_workingCosmos = data.owner;
    }
    
    void ParallelNetworkDynamo::_timestream_interception_handler(EventMessage interceptionEvent)
    {

        std::shared_ptr<Cosmos> cosmos = m_workingCosmos.lock();
        if (m_workingCosmos.expired()) return;

        events::cosmos::TIMESTREAM_INTERCEPTION_params interceptionInfo;
        interceptionEvent >> interceptionInfo;

        // Promote directly to forked
        cosmos->set_timestream_state(interceptionInfo.recipient, TimestreamState::forked);
    }
  
    
    void ParallelNetworkDynamo::_parallel_init_handler(EventMessage initEvent)
    {
        events::parallel::INIT_params initInfo;
        initEvent >> initInfo;
        initEvent << initInfo;
        // just forward to source timeslice
        m_timelineApi.send_message(initInfo.sourceTimeslice, initEvent);
    }

    void ParallelNetworkDynamo::_parallel_finished_handler(EventMessage finishedEvent)
    {
        events::parallel::FINISHED_params finishedInfo;
        finishedEvent >> finishedInfo;
        finishedEvent << finishedInfo;
        // just forward to destination timeslice
        m_timelineApi.send_message(finishedInfo.destinationTimeslice, finishedEvent);
    }
}
