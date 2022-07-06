#ifndef NET_I_SERVER_H
#define NET_I_SERVER_H

//#include "intercession_pch.h"
#include <memory>

#include "logging/pleep_log.h"
#include "networking/ts_queue.h"
#include "networking/net_message.h"
#include "networking/net_connection.h"

namespace pleep
{
namespace net
{
    // Server interface for connections of a single message type
    template<typename MsgType>
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
            try
            {
                // issue "work" to asio context
                this->wait_for_connection();
                // then start context on separate thread
                m_contextThread = std::thread([this]() { m_asioContext.run() });
            }
            catch(const std::exception& err)
            {
                UNREFERENCED_PARAMETER(err);
                PLEEPLOG_ERROR("Asio failure: " + err.what());
                //throw err;
                return false;
            }

            PLEEPLOG_TRACE("Started looking for connections!");
            return true;
        }

        void stop()
        {
            m_asioContext.stop();

            if (m_contextThread.joinable()) m_contextThread.join();
            
            PLEEPLOG_TRACE("Stopped looking for connections.");
        }

        // ASYNC
        void wait_for_connection()
        {
            m_asioAcceptor.acync_accept(
                [this](std::error_code ec, asio::ip::tcp::socket socket)
                {
                    if (!ec)
                    {
                        PLEEPLOG_TRACE("New connection: " + std::to_string(socket.remote_endpoint()));

                        std::shared_ptr<Connection<MsgType>> newConn = std::make_shared<Connection<MsgType>>(Connection<MsgType::owner::server, m_asioContext, std::move(socket), m_incomingMessages);

                        // go to on connect callback
                        if (this->on_remote_connect(newConn))
                        {
                            m_connectionDeque.push_back(std::move(newConn));

                            m_connectionsDeque.back()->connect_to_client(m_idCounter++);

                            PLEEPLOG_TRACE("[" + std::to_string(m_connectionDeque.back()->get_id()) + "] Connection approved");
                        }
                        else
                        {
                            PLEEPLOG_TRACE("Connection denied");
                        }
                    }
                    else
                    {
                        PLEEPLOG_ERROR("Asio error during connection accept: " + ec.message());
                    }

                    // start waiting again in loop
                    this->wait_for_connection();
                }
            );
        }

        void send_message(std::shared_ptr<Connection<MsgType>> remote, const Message<MsgType>& msg)
        {
            if (remote && remote->is_connected())
            {
                remote->send(msg);
            }
            // remote is no longer valid since last communication... assume it is disconnected
            else
            {
                this->on_remote_disconnect(remote);
                remote.reset();
                m_connectionDeque.erase(
                    std::remove(m_connectionDeque.begin(), m_connectionDeque.end(), remote),
                    m_connectionDeque.end()
                )
            }
        }

        // option to ignore 1 particular connection?
        void broadcast_message(const Message<MsgType>& msg, std::shared_ptr<Connection<MsgType>> ignoredConnection = nullptr)
        {
            // remember invalid remote after iterating
            bool invalidRemoteExists = false;

            for (auto& remote : m_connectionDeque)
            {
                if (remote && remote->is_connected())
                {
                    if (remote != ignoredConnection)
                        remote->send(msg);
                }
                else
                {
                    this->on_remote_disconnect(remote);
                    remote.reset();
                    invalidRemoteExists = true;
                }
            }

            if (invalidRemoteExists)
            {
                m_connectionDeque.erase(
                    std::remove(m_connectionDeque.begin(), m_connectionDeque.end(), nullptr),
                    m_connectionDeque.end()
                )
            }
        }

        // process incoming messages explicitly (instead of async callbacks)
        // only process a max number before moving on to next frame ((unsigned)-1 == MAX value)
        void update(size_t maxMessages = -1)
        {
            size_t messageCount = 0;
            while (messageCount < maxMessages && !m_incomingMessages.empty())
            {
                auto msg = m_incomingMessages.pop_front();

                this->on_message(msg.remote, msg.msg);

                messageCount++;
            }
        }

    protected:
        // Called to when a remote connects to us. Returning false denies the connection
        virtual bool on_remote_connect(std::shared_ptr<Connection<MsgType>> remote)
        {
            return false;
        }

        // Called when a remote "appears" to have disconnected
        virtual void on_remote_disconnect(std::shared_ptr<Connection<MsgType>> remote)
        {

        }

        // Called when a message arrives in the queue
        virtual void on_message(std::shared_ptr<connection<MsgType>> remote, Message<MsgType>& msg)
        {

        }

        // A queue for all messages of the designated server type
        // All my related connections will share this queue
        TsQueue<OwnedMessage<MsgType>> m_incomingMessages;

        // Maintain container of established connections
        std::deque<std::shared_ptr<Connection<MsgType>>> m_connectionDeque;

        // NOTE: Order of declaration = order of initialisation
        // context shared across all connections
        asio::io_context m_asioContext;
        std::thread m_contextThread;
        
        asio::ip::tcp::acceptor m_asioAcceptor;

        // interally maintained numberical id for each connection
        uint32_t m_idCounter = 1000;
    };
}
}

#endif // NET_I_SERVER_H