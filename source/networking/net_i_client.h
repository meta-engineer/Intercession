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
                std::ostringstream contextThreadId; contextThreadId << m_contextThread.get_id();
                PLEEPLOG_INFO("Constructed network thread #" + contextThreadId.str());
            }
            catch (std::exception& err)
            {
                UNREFERENCED_PARAMETER(err);
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

            // stop the previous run() call
            m_asioContext.stop();
            if (m_contextThread.joinable())
            {
                m_contextThread.join();
            }

            // re-prime context
            m_asioContext.restart();
            // block and run a few(?) times to try to clear any jobs (at least 1 for async_read and async_write?)
            for (uint8_t i = 0U; i < 10U; i++)
            {
                if (m_asioContext.run_one() == 0U) break;
            }
            // last run_one failed, so we need to prime context again
            m_asioContext.restart();
            
            // destroy member reference to connection
            m_connection = nullptr;
            // remove all shared queue references to connection to invoke destructor
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

        // provide user access to m_incomingMessages synchronously
        bool is_message_available()
        {
            return !(m_incomingMessages.empty());
        }
        // returns false if nothing was available
        bool pop_message(Message<T_Msg>& dest)
        {
            // Do we want to return the owned message for convenience? (even though there is only 1 connection)
            OwnedMessage<T_Msg> ownedDest;
            bool res = m_incomingMessages.pop_front(ownedDest);
            if (res) dest = ownedDest.msg;

            return res;
        }

    protected:
        // messages recieved from connected "remote"
        TsQueue<OwnedMessage<T_Msg>> m_incomingMessages;

        // root context shared with connections
        asio::io_context m_asioContext;
        // thread for asio context
        std::thread m_contextThread;
        // client interface has single connection
        std::shared_ptr<Connection<T_Msg>> m_connection = nullptr;
    };
}
}

#endif // NET_I_CLIENT_H