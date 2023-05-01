#ifndef NET_I_SERVER_H
#define NET_I_SERVER_H

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
    // Server interface for connections of a single message type
    template<typename T_Msg>
    class I_Server
    {
    protected:
        I_Server(uint16_t port)
            : m_asioAcceptor(m_asioContext, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port))
        {
        }
    public:
        virtual ~I_Server()
        {
            this->stop();
        }

        bool start()
        {
            PLEEPLOG_TRACE("Starting to wait for connections!");
            try
            {
                // issue "work" to asio context
                this->_wait_for_connection();
                // then start context on separate thread
                m_contextThread = std::thread([this]() { m_asioContext.run(); });
                std::ostringstream contextThreadId; contextThreadId << m_contextThread.get_id();
                PLEEPLOG_INFO("Constructed network thread #" + contextThreadId.str());
            }
            catch(const std::exception& err)
            {
                UNREFERENCED_PARAMETER(err);
                PLEEPLOG_ERROR("Asio runtime failure:");
                PLEEPLOG_ERROR(err.what());
                //throw err;
                return false;
            }
            
            return true;
        }

        void stop()
        {
            m_asioContext.stop();

            if (m_contextThread.joinable()) m_contextThread.join();
            
            PLEEPLOG_TRACE("Stopped looking for connections.");
        }

        void send_message(std::shared_ptr<Connection<T_Msg>> remote, const Message<T_Msg>& msg)
        {
            if (!remote) return;
            // remote is no longer valid since last communication... ASSUME it disconnected ungracefully
            if (!remote->is_connected())
            {
                PLEEPLOG_DEBUG("[" + std::to_string(remote->get_id()) + "] Found invalid connection to cleanup");
                this->on_remote_disconnect(remote);
                remote.reset();
                m_connectionDeque.erase(
                    std::remove(m_connectionDeque.begin(), m_connectionDeque.end(), remote),
                    m_connectionDeque.end()
                );
            }
            // otherwise try to send message (send will fail safely if connection is not ready/initialized)
            else
            {
                remote->send(msg);
            }
        }

        // option to ignore 1 particular connection?
        void broadcast_message(const Message<T_Msg>& msg, std::shared_ptr<Connection<T_Msg>> ignoredConnection = nullptr)
        {
            // remember invalid remote after iterating
            bool invalidRemoteExists = false;

            for (auto& remote : m_connectionDeque)
            {
                if (!remote) continue;
                // remote is no longer valid since last communication... ASSUME it disconnected ungracefully
                if (!remote->is_connected())
                {
                    PLEEPLOG_DEBUG("[" + std::to_string(remote->get_id()) + "] Found invalid connection to cleanup");
                    this->on_remote_disconnect(remote);
                    remote.reset();
                    invalidRemoteExists = true;
                }
                // otherwise try to send message (send will fail safely if connection is not ready/initialized)
                else
                {
                    if (remote != ignoredConnection) 
                        remote->send(msg);
                }
            }

            if (invalidRemoteExists)
            {
                m_connectionDeque.erase(
                    std::remove(m_connectionDeque.begin(), m_connectionDeque.end(), nullptr),
                    m_connectionDeque.end()
                );
            }
        }

        // provide user access to m_incomingMessages synchronously
        bool is_message_available()
        {
            return !(m_incomingMessages.empty());
        }
        OwnedMessage<T_Msg> pop_message()
        {
            return m_incomingMessages.pop_front().first;
        }

    private:
        // ASYNC - Task asio to wait for connection
        // asio will provide us with the connected socket to use to append m_connectionDeque
        void _wait_for_connection()
        {
            m_asioAcceptor.async_accept(
                [this](std::error_code ec, asio::ip::tcp::socket socket)
                {
                    if (!ec)
                    {
                        PLEEPLOG_DEBUG("New connection: " + socket.remote_endpoint().address().to_string() + ":" + std::to_string(socket.remote_endpoint().port()));

                        std::shared_ptr<Connection<T_Msg>> newConn = std::make_shared<Connection<T_Msg>>(m_asioContext, std::move(socket), m_incomingMessages);

                        // go to on connect callback
                        if (this->on_remote_connect(newConn))
                        {
                            m_connectionDeque.push_back(std::move(newConn));
                            m_connectionDeque.back()->set_id(m_idCounter++);
                            PLEEPLOG_DEBUG("[" + std::to_string(m_connectionDeque.back()->get_id()) + "] Connection approved");

                            m_connectionDeque.back()->start_communication(this);
                        }
                        else
                        {
                            PLEEPLOG_DEBUG("[----] Connection denied");
                        }
                    }
                    else
                    {
                        PLEEPLOG_ERROR("Asio error during connection accept: " + ec.message());
                    }

                    // start waiting again in loop
                    this->_wait_for_connection();
                }
            );
        }

    protected:
        // Called to when a remote connects to us. Returning false denies the connection
        virtual bool on_remote_connect(std::shared_ptr<Connection<T_Msg>> remote)
        {
            UNREFERENCED_PARAMETER(remote);
            return false;
        }

        // Called when a remote "appears" to have disconnected
        virtual void on_remote_disconnect(std::shared_ptr<Connection<T_Msg>> remote)
        {
            UNREFERENCED_PARAMETER(remote);
        }

        // Called by Connection when a remote has passed validation.
        friend class Connection<T_Msg>;
        virtual void on_remote_validated(std::shared_ptr<Connection<T_Msg>> remote)
        {
            UNREFERENCED_PARAMETER(remote);
        }

        // A queue for all messages of the designated server type
        // All my related connections will share this queue
        TsQueue<OwnedMessage<T_Msg>> m_incomingMessages;

    private:
        // Maintain container of established connections
        std::deque<std::shared_ptr<Connection<T_Msg>>> m_connectionDeque;

        // NOTE: Order of declaration = order of initialisation
        // context shared across all connections
        asio::io_context m_asioContext;
        std::thread m_contextThread;
        
        // set task to build Connection upon incoming connection
        asio::ip::tcp::acceptor m_asioAcceptor;

        // interally maintained numberical id for each connection
        uint32_t m_idCounter = 1000;
    };
}
}

#endif // NET_I_SERVER_H