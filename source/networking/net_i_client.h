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

                m_connection = std::make_shared<Connection<T_Msg>>(
                    m_asioContext,
                    asio::ip::tcp::socket(m_asioContext), 
                    m_incomingMessages
                );

                m_connection->connect_to_remote(endpoints);

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

            // remove member reference to connection
            m_connection = nullptr;

            // remove all queue references to connection
            // to invoke destructor
            m_incomingMessages.clear();
        }

        // passthrough connection status
        bool is_connected()
        {
            return m_connection && m_connection->is_connected();
        }

        // passthrough connection status
        bool is_ready()
        {
            return m_connection && m_connection->is_ready();
        }

        void send_message(const Message<T_Msg>& msg)
        {
            if (!this->is_connected())
            {
                PLEEPLOG_ERROR("Cannot send message, Conection is closed");
            }
            else
            {
                m_connection->send(msg);
            }
        }

        // process incoming messages explicitly at a known time (instead of async callbacks)
        // only process a max number before moving on to next frame ((unsigned)-1 == MAX value)
        void process_received_messages(size_t maxMessages = -1)
        {
            size_t messageCount = 0;
            while (messageCount < maxMessages && !m_incomingMessages.empty())
            {
                OwnedMessage<T_Msg> msg = m_incomingMessages.pop_back().first;

                this->on_message(msg.msg);

                messageCount++;
            }
        }

    protected:
        virtual void on_message(Message<T_Msg>& msg)
        {
            UNREFERENCED_PARAMETER(msg);
        }

        // root context shared with connections
        asio::io_context m_asioContext;
        // thread for asio context
        std::thread m_contextThread;
        // client interface has single connection
        std::shared_ptr<Connection<T_Msg>> m_connection;

    private:
        // messages recieved from connected "remote"
        TsQueue<OwnedMessage<T_Msg>> m_incomingMessages;
    };
}
}

#endif // NET_I_CLIENT_H