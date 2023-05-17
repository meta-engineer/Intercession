#include "server_network_dynamo.h"

#include "logging/pleep_log.h"
#include "ecs/ecs_types.h"
#include "staging/cosmos_builder.h"

namespace pleep
{
    ServerNetworkDynamo::ServerNetworkDynamo(EventBroker* sharedBroker, TimelineApi localTimelineApi)
        : I_NetworkDynamo(sharedBroker)
        , m_timelineApi(localTimelineApi)
        , m_networkApi(m_timelineApi.get_port())
    {
        PLEEPLOG_TRACE("Start server networking pipeline setup");

        // ***** TESTING *****
        if (m_timelineApi.get_timeslice_id() == 0)
        {
            PLEEPLOG_DEBUG("I am origin timeslice (0), sending a test message to timeslice 1 via timeline api");
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

        PLEEPLOG_TRACE("Done server networking pipeline setup");
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
                PLEEPLOG_DEBUG("Received ENTITY_CREATED message for Entity " + std::to_string(data.entity) + " directly from timeslice " + std::to_string(derive_timeslice_id(data.entity)));
                // double check if we are it's host
                if (derive_timeslice_id(data.entity) != m_timelineApi.get_timeslice_id())
                {
                    PLEEPLOG_WARN("Received a ENTITY_CREATED signal for an entity we don't host. Is this intentional?");
                }
                else
                {
                    m_workingCosmos->increment_hosted_temporal_entity_count(data.entity);
                }
            }
            break;
            case events::cosmos::ENTITY_REMOVED:
            {
                events::cosmos::ENTITY_REMOVED_params data;
                msg >> data;
                PLEEPLOG_DEBUG("Received ENTITY_REMOVED message for Entity " + std::to_string(data.entity) + " directly from timeslice " + std::to_string(derive_timeslice_id(data.entity)));
                // double check if we are it's host
                if (derive_timeslice_id(data.entity) != m_timelineApi.get_timeslice_id())
                {
                    PLEEPLOG_WARN("Received a ENTITY_REMOVED signal for an entity we don't host. Is this intentional?");
                }
                else
                {
                    m_workingCosmos->decrement_hosted_temporal_entity_count(data.entity);
                }
            }
            break;
            default:
            {
                PLEEPLOG_DEBUG("Received unknown message id " + std::to_string(msg.header.id) + " direct from another(?) timeslice");
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
            case events::cosmos::ENTITY_UPDATE:
            {
                PLEEPLOG_DEBUG("[" + std::to_string(msg.remote->get_id()) + "] Received entity update message");
                // what do we do with data now?
                //m_entityUpdateRelay->submit(msg.msg);
            }
            break;
            case events::network::NEW_CLIENT:
            {
                PLEEPLOG_DEBUG("[" + std::to_string(msg.remote->get_id()) + "] Received new client notice");

                // TODO: send app_info? then Intercession_info?

                // Send Cosmos config
                // should be stateless and scan current workingCosmos?
                // How should scan work, just store config into Cosmos when it is built? What if it changes afterwards?
                // If CosmosBuilder could use synchro/component typeid then we could scan Cosmos' registries
                // CosmosBuilder::Config needs to be serializable... are std::sets serializable?

                PLEEPLOG_DEBUG("New client joined, I should send them a cosmos config!");
                /* Message<EventId> configMsg(events::cosmos::CONFIG);
                events::cosmos::CONFIG_params configInfo;
                CosmosBuilder scanner;
                configInfo.config = scanner.scan(m_workingCosmos);
                configMsg << configInfo;
                msg.remote->send(configMsg); */

                PLEEPLOG_DEBUG("Setup Entity for new client to control?");
                // Should client have control of how the entity is created?
                // or should the server control how the entity is created?

                // Send initialization for all entities
                for (auto signIt : m_workingCosmos->get_signatures_ref())
                {
                    EventMessage createMsg(events::cosmos::ENTITY_CREATED);
                    events::cosmos::ENTITY_CREATED_params createInfo = {signIt.first, signIt.second};
                    createMsg << createInfo;
                    PLEEPLOG_DEBUG("Sending message: " + createMsg.info());
                    msg.remote->send(createMsg);
                }

                // Forward new client message to signal for cosmos to initialize client side entities
                msg.remote->send(msg.msg);
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
        //m_entityUpdateRelay->engage(deltaTime);

        // Fifth: After all ingesting is done, send/broadcast fresh data to timestreams & clients
        // TODO: how do we compensate for latency?
        for (auto signIt : m_workingCosmos->get_signatures_ref())
        {
            EventMessage updateMsg(events::cosmos::ENTITY_UPDATE);
            m_workingCosmos->serialize_entity_components(signIt.first, updateMsg);
            events::cosmos::ENTITY_UPDATE_params updateInfo = { signIt.first, signIt.second };
            updateMsg << updateInfo;
            m_networkApi.broadcast_message(updateMsg);
        }
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
        // Entity created locally. If we have a child timeslice, that means next frame this entity will enter the past timestream
        // so we need to increment its host's count
        events::cosmos::ENTITY_CREATED_params newEntityParams;
        entityEvent >> newEntityParams;
        entityEvent << newEntityParams; // re-fill message for forwarding
        if (m_timelineApi.has_past())
        {
            TimesliceId host = derive_timeslice_id(newEntityParams.entity);
            m_timelineApi.send_message(host, entityEvent);

            // TODO: we should also emplace in the past timestream now and add the ENTITY_CREATED message
        }
    }
    
    void ServerNetworkDynamo::_entity_removed_handler(EventMessage entityEvent)
    {
        // entity has left our timeslice so we need to decrement its host's count
        
        events::cosmos::ENTITY_REMOVED_params removedEntityParams;
        entityEvent >> removedEntityParams;
        TimesliceId host = derive_timeslice_id(removedEntityParams.entity);


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