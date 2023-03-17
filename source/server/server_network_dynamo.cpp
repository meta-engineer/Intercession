#include "server_network_dynamo.h"

#include "logging/pleep_log.h"
#include "networking/timeline_types.h"

namespace pleep
{
    ServerNetworkDynamo::ServerNetworkDynamo(EventBroker* sharedBroker, TimelineApi localTimelineApi)
        : I_NetworkDynamo(sharedBroker)
        , m_timelineApi(localTimelineApi)
        , m_networkApi(m_timelineApi.get_port())
    {
        PLEEPLOG_TRACE("Setup Server Networking pipeline");

        // ***** TESTING *****
        if (m_timelineApi.get_timeslice_id() == 0)
        {
            PLEEPLOG_DEBUG("I am origin timeslice (0), sending a test message to timeslice 1");
            EventMessage testMsg(events::network::INTERCESSION_UPDATE);
            m_timelineApi.send_message(1, testMsg);
        }
        // ***** TESTING *****

        // setup relays
        m_entityUpdateRelay = std::make_unique<ServerEntityUpdateRelay>();

        // start listening on asio server
        m_networkApi.start();

        
        // setup handlers
        m_sharedBroker->add_listener(METHOD_LISTENER(events::cosmos::ENTITY_CREATED, ServerNetworkDynamo::_entity_created_handler));
        m_sharedBroker->add_listener(METHOD_LISTENER(events::cosmos::ENTITY_REMOVED, ServerNetworkDynamo::_entity_removed_handler));

        PLEEPLOG_TRACE("Done Server Networking pipeline setup");
    }
    
    ServerNetworkDynamo::~ServerNetworkDynamo() 
    {
        
    }
    
    void ServerNetworkDynamo::run_relays(double deltaTime) 
    {
        UNREFERENCED_PARAMETER(deltaTime);
        // Handle async data from network and local (timelineApi)
        // handling paradigms should ideally be the same:
        // let their respective threads accumulate messages,
        // then during run_relays we will pop them and handle them here

        // Network/Local Api should only handle specific internal logic for themselves
        // which they can do so immediately on their own thread
        //   (e.g. managing connections)
        // And then store necessary data for us to invoke cosmos specific logic later
        //   (e.g. submittions to relays, or events)
        // dynamo could call with message data/event as a "packet" (though not the strict definition of a Cosmos packet)

        // Ordering of methods?
        // First: process DIRECT messages from other timeslices
        //_process_timeline_messages();
        while(m_timelineApi.is_message_available())
        {
            PLEEPLOG_TRACE("I received a message from another timeslice!");
            EventMessage msg = m_timelineApi.pop_message();
            switch (msg.header.id)
            {
            case events::cosmos::ENTITY_CREATED:
            {
                events::cosmos::ENTITY_CREATED_params data;
                msg >> data;
                PLEEPLOG_DEBUG("Received ENTITY_CREATED message for temporalEntity " + std::to_string(data.temporalEntity) + " directly from timeslice " + std::to_string(extract_host_timeslice_id(data.temporalEntity)));
                // double check if we are it's host
                if (extract_host_timeslice_id(data.temporalEntity) != m_timelineApi.get_timeslice_id())
                {
                    PLEEPLOG_WARN("Received a ENTITY_CREATED signal for an entity we don't host. Is this intentional?");
                }
                else
                {
                    m_workingCosmos->increment_hosted_temporal_entity_count(data.temporalEntity);
                }
            }
            break;
            case events::cosmos::ENTITY_REMOVED:
            {
                events::cosmos::ENTITY_REMOVED_params data;
                msg >> data;
                PLEEPLOG_DEBUG("Received ENTITY_REMOVED message for temporalEntity " + std::to_string(data.temporalEntity) + " directly from timeslice " + std::to_string(extract_host_timeslice_id(data.temporalEntity)));
                // double check if we are it's host
                if (extract_host_timeslice_id(data.temporalEntity) != m_timelineApi.get_timeslice_id())
                {
                    PLEEPLOG_WARN("Received a ENTITY_REMOVED signal for an entity we don't host. Is this intentional?");
                }
                else
                {
                    m_workingCosmos->decrement_hosted_temporal_entity_count(data.temporalEntity);
                }
            }
            break;
            default:
            {
                PLEEPLOG_DEBUG("Received unknown message direct from another (?) timeslice");
            }
            break;
            }
        }

        // Second: handle incoming timestream messages from other timeslices
        // This should get our cosmos as close as possible to realtime
        //_process_timestream_messages();
        // TODO: how do we know which entities to pop messages from...

        // Third: handle incoming client messages from network
        //_process_network_messages();
        const size_t maxMessages = static_cast<size_t>(-1);
        size_t messageCount = 0;
        while ((messageCount++) < maxMessages && m_networkApi.is_message_available())
        {
            net::OwnedMessage<EventId> msg = m_networkApi.pop_message();
            switch (msg.msg.header.id)
            {
            case events::network::INTERCESSION_UPDATE:
            {
                PLEEPLOG_DEBUG("[" + std::to_string(msg.remote->get_id()) + "] Bouncing intercession update message");
                // just bounce back
                msg.remote->send(msg.msg);
            }
            break;
            case events::network::ENTITY_UPDATE:
            {
                PLEEPLOG_DEBUG("[" + std::to_string(msg.remote->get_id()) + "] Received entity update message");
                // what do we do with data now?
                //m_entityUpdateRelay->submit(msg.msg);
            }
            break;
            default:
            {
                PLEEPLOG_DEBUG("[" + std::to_string(msg.remote->get_id()) + "] Recieved unknown message");
            }
            break;
            }
        }

        // Fourth: Process all submitted data in relays
        m_entityUpdateRelay->engage(deltaTime);

        // Fifth: After all ingesting is done, send/broadcast fresh data to timestreams
        // TODO: Fetch all temporal entities and feed updates into timelineApi
    }
    
    void ServerNetworkDynamo::reset_relays() 
    {
        // clear our working cosmos
        m_workingCosmos = nullptr;

        // AND clear relay working cosmos and leftover submittions
        m_entityUpdateRelay->clear();
    }

    void ServerNetworkDynamo::submit(CosmosAccessPacket data) 
    {
        // save our own working cosmos for our event handling
        m_workingCosmos = data.owner;

        // pass working cosmos to relays
        m_entityUpdateRelay->submit(data);
    }
    
    TimesliceId ServerNetworkDynamo::get_timeslice_id()
    {
        return m_timelineApi.get_timeslice_id();
    }
    
    void ServerNetworkDynamo::_entity_created_handler(EventMessage entityEvent)
    {
        // if we have a child timeslice, that means next frame this entity will enter the timestream
        // so we need to increment its host's count

        // TODO: we should also emplace the timestream now and add the ENTITY_CREATED message
        
        events::cosmos::ENTITY_CREATED_params newEntityParams;
        entityEvent >> newEntityParams;

        if (m_timelineApi.has_past())
        {
            TimesliceId host = extract_host_timeslice_id(newEntityParams.temporalEntity);
            m_timelineApi.send_message(host, entityEvent);
        }
    }
    
    void ServerNetworkDynamo::_entity_removed_handler(EventMessage entityEvent)
    {
        // entity has left our timeslice so we need to decrement its host's count
        
        events::cosmos::ENTITY_REMOVED_params removedEntityParams;
        entityEvent >> removedEntityParams;
        TimesliceId host = extract_host_timeslice_id(removedEntityParams.temporalEntity);


        m_timelineApi.send_message(host, entityEvent);

        if (m_timelineApi.has_past())
        {
            // TODO: we should also propogate the removal through our timestream so that
            //   our child can remove aswell
            // should timstream index on entity of temporalId?
            //m_timelineApi.push_past_timestream();
        }
        
    }
}