#ifndef I_NETWORK_DYNAMO_H
#define I_NETWORK_DYNAMO_H

//#include "intercession_pch.h"
#include "core/a_dynamo.h"
#include "core/cosmos_access_packet.h"

namespace pleep
{
    // Instead of operating like a normal dynamo (where a synchro submits entity packets to be updated)
    // it might make more sense to give the network dynamo direct access to the ecs and it can update
    // only the entities with new data (not all entities may have new data to update and will just be skipped)
    // Entities may need to have a NetworkComponent to maintain some states/persistence?

    // We also need the network dynamo to know which entities it needs to "report"
    // servers will have to report every* entity, but clients will only report those who've directly
    // been manipulated by the user (control dynamo?)
    // We'll have the control dynamo broadcast an event every time it updates an entity
    // which client network dynamos can catch and build their update list

    // when we send/receive an entity, which components should be communicated? likely all of them
    // but from cosmos we can only get an entity's signature, we don't know what components are represented
    // aka which components have been registered
    // we need some way to unpack and/or feed the data to the cosmos to be written into the ecs

    class I_NetworkDynamo : public A_Dynamo
    {
    protected:
        // passthrough to A_Dynamo (can't call A_Dynamo's constructor directly from 2 layers deep)
        I_NetworkDynamo(std::shared_ptr<EventBroker> sharedBroker) 
            : A_Dynamo(sharedBroker)
        {}

    public:
        // provide access to entire ecs to apply updates to entities only as needed
        virtual void submit(CosmosAccessPacket data) = 0;
        
        // passthrough to timelineApi for servers, always null for clients
        virtual TimesliceId get_timeslice_id()
        {
            return NULL_TIMESLICEID;
        }

        // for servers: number of active clients
        // for clients: either 1 or 0 if connected to a server or not
        virtual size_t get_num_connections()
        {
            return 0;
        }

        // for servers: return my app info
        // for clients: return connected server's app info
        virtual events::network::APP_INFO_params get_app_info()
        {
            return {};
        }

        // for servers: close all connections and reopen with address/port
        // for clients: close connection and try to reconnect to address/port
        virtual void restart_connection(const std::string& address, uint16_t port)
        {
            UNREFERENCED_PARAMETER(address);
            UNREFERENCED_PARAMETER(port);
            return;
        }
    };
}

#endif // I_NETWORK_DYNAMO_H