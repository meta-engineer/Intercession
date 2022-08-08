#ifndef PLEEP_NET_H
#define PLEEP_NET_H

//#include "intercession_pch.h"
#include <cstdint>

#include "build_config.h"
#include "networking/net_i_server.h"
#include "networking/net_i_client.h"

namespace pleep
{
namespace net
{
    // TODO: standardize event/message types
    // enum or namespaces???
    // define some messagetype enum
    enum class PleepMessageType : uint32_t
    {
        null,
        appInfo,
        intercessionInfo,
        cosmosInfo,
        entityUpdate,
        intercession
    };

    // Generic/basic app properties
    // Should probably be only info generic to all potential apps for consisstent size
    struct AppInfo
    {
        const char* name = BUILD_PROJECT_NAME;
        const uint8_t versionMajor = BUILD_VERSION_MAJOR;
        const uint8_t versionMinor = BUILD_VERSION_MINOR;
        const uint8_t versionPatch = BUILD_VERSION_PATCH;
    };

    // Info about the Intercession app configuration specifically:
    //  Am I a server of a client (or a dispatch)
    //  What is the ID of my cluster/server group?
    //  How many servers are in my cluster?
    //  what is my client id/server id
    //  What is the address of the "lead" server
    struct IntercessionInfo
    {

    };

    // Info about the current cosmos:
    //  Running state
    //  total registered synchros, components, and entities
    //  checksum of registered synchros & components
    struct CosmosInfo
    {

    };

    // entity update will by a dynamically packed series of components:
    // first the EntityId (32 bits), then the entity signature (32 bits)
    // and then each consecutive component represented in the signature.
    // assuming the IntercessionInfo checks out the component layout should match

    class PleepServer : public I_Server<PleepMessageType>
    {
    public:
        PleepServer(uint16_t port)
            : I_Server<PleepMessageType>(port)
        {
        }

    protected:
        bool on_remote_connect(std::shared_ptr<Connection<PleepMessageType>> remote) override
        {
            UNREFERENCED_PARAMETER(remote);
            PLEEPLOG_DEBUG("Checking new connection: " + remote->get_endpoint().address().to_string() + ":" + std::to_string(remote->get_endpoint().port()));
            return true;
        }
        
        void on_remote_validated(std::shared_ptr<Connection<PleepMessageType>> remote) override
        {
            UNREFERENCED_PARAMETER(remote);
            PLEEPLOG_DEBUG("[" + std::to_string(remote->get_id()) + "] Checking validated connection");
        }
        
        void on_remote_disconnect(std::shared_ptr<Connection<PleepMessageType>> remote) override
        {
            UNREFERENCED_PARAMETER(remote);
            PLEEPLOG_DEBUG("[" + std::to_string(remote->get_id()) + "] Found invalid connection to cleanup");

        }
        
        void on_message(std::shared_ptr<Connection<PleepMessageType>> remote, Message<PleepMessageType>& msg) override
        {
            UNREFERENCED_PARAMETER(remote);
            UNREFERENCED_PARAMETER(msg);

            switch (msg.header.id)
            {
            case net::PleepMessageType::intercession:
            {
                PLEEPLOG_DEBUG("[" + std::to_string(remote->get_id()) + "] Bouncing intercession message");
                // just bounce back
                remote->send(msg);
            }
            break;
            default:
            {
                PLEEPLOG_DEBUG("[" + std::to_string(remote->get_id()) + "] Recieved unknown message");
            }
            break;
            }

        }
    };

    class PleepClient : public I_Client<PleepMessageType>
    {

    };

    void test_net();
}
}

#endif // PLEEP_NET_H