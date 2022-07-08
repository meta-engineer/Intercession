#ifndef NET_I_CLIENT_H
#define NET_I_CLIENT_H

//#include "intercession_pch.h"
#include <string>
#include <memory>
#include <thread>
#include <algorithm>

#include "logging/pleep_log.h"
#include "networking/ts_queue.h"
#include "networking/net_message.h"
#include "networking/net_connection.h"

namespace pleep
{
namespace net
{
    template <typename T_Msg>
    class I_Client
    {
    protected:
        I_Client()
        {
            // construct asio socket with context
        }
    public:
        virtual ~I_Client()
        {
            disconnect();
        }

        // resolve and connect to an ambiguous endpoint
        bool connect(const std::string& host, const uint16_t port)
        {
            try
            {
                // Convert host to tangiable physical address
                asio::ip::tcp::resolver asioResolver(m_asioContext);
                // store endpoints for interrogation later?
                asio::ip::tcp::resolver::results_type endpoints = asioResolver.resolve(host, std::to_string(port));

                m_connection = std::make_unique<Connection<T_Msg>>(
                    Connection<T_Msg>::owner::client, 
                    m_asioContext,
                    asio::ip::tcp::socket(m_asioContext), 
                    m_incomingMessages
                );

                m_connection->connect_to_server(endpoints);

                // start async asio thread
                m_contextThread = std::thread([this]() { m_asioContext.run(); });
            }
            catch (std::exception& err)
            {
                PLEEPLOG_ERROR("Asio error while connecting: ");
                PLEEPLOG_ERROR(err.what());
                return false;
            }

            return true;
        }

        void disconnect()
        {
            if (this->is_connected())
            {
                m_connection->disconnect();
            }

            m_asioContext.stop();
            if (m_contextThread.joinable())
                m_contextThread.join();

            // invoke destructor of connection
            m_connection.release();
        }

        bool is_connected()
        {
            if (m_connection)
                return m_connection->is_connected();
            else
                return false;
        }

        void send_message(const Message<T_Msg>& msg)
        {
            if (this->is_connected())
            {
                m_connection->send(msg);
            }
        }

        // public accessor for message queue
        // TODO: should client have an update/on_message pattern like server?
        TsQueue<OwnedMessage<T_Msg>>& get_incoming_messages()
        {
            return m_incomingMessages;
        }

    protected:
        // root context shared with connections
        asio::io_context m_asioContext;
        // thread for asio context
        std::thread m_contextThread;
        // client interface has single connection
        std::unique_ptr<Connection<T_Msg>> m_connection;

    private:
        // messages recieved from connected "remote"
        TsQueue<OwnedMessage<T_Msg>> m_incomingMessages;
    };
}
}

#endif // NET_I_CLIENT_H