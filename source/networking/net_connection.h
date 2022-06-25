#ifndef NET_CONNECTION_H
#define NET_CONNECTION_H

//#include "intercession_pch.h"
#ifdef _WIN32
#define _WIN32_WINNT 0x0A00
#endif
#define ASIO_STANDALONE
#include <asio.hpp>
#include <asio/ts/buffer.hpp>
#include <asio/ts/internet.hpp>

#include "networking/ts_queue.h"
#include "networking/net_message.h"

namespace pleep
{
namespace net
{
    template<typename MsgType>
    class Connection : public std::enable_shared_from_this<connection<MsgType>>
    {
    public:
        enum class owner
        {
            server,
            client
        };

        Connection(owner parent, asio::io_context& asioContext, asio::ip::tcp::socket socket, TsQueue<OwnedMessage<MsgType>>& inQ)
            : m_asioContext(asioContext)
            , m_socket(std::move(socket))
            , m_incomingMessages(inQ)
        {
            m_ownerType = parent;
        }
        ~Connection()
        {}

        uint32_t get_id() const
        {
            return m_id;
        }

        bool connect_to_client(uint32_t id = 0)
        {
            if (m_ownerType == owner::server)
            {
                if (m_socket.is_open())
                {
                    m_id = id;

                    // prime to read
                    this->read_header();
                }
            }
        }
        bool connect_to_server(const asio::ip::tcp::resolver::results_type& endpoints)
        {
            // only clients can do this
            if (m_ownerType == owner::client)
            {
                // Attempt to connect
                asio::async_connect(m_socket, endpoints,
                    [this](std::error_code ec, asio::ip::tcp::endpoint endpoint)
                    {
                        if (!ec)
                        {
                            // now we're connected, prime to read header
                            this->read_header();
                        }
                        else
                        {
                            PLEEPLOG_ERROR("Asio error: " + ec.message());
                            // on error close socket to that server/client will cleanup
                            m_socket.close();
                        }
                    }
                );
            }
        }

        bool disconnect()
        {
            if (this->is_connected())
            {
                asio::post(m_asioContext, [this]() { m_socket.close() });
            }
        }

        bool is_connected() const
        {
            return m_socket.is_open();
        }

        bool send(const Message<MsgType>& msg)
        {
            // send a "job" to the context
            asio::post(
                m_asioContext,
                [this, msg]()
                {
                    // check if messages are already being written
                    bool isWriting = !m_outgoingMessages.empty();

                    m_outgoingMessages.push_back(msg);

                    // prime for writing if not already
                    if (!isWriting)
                    {
                        this->write_header();
                    }
                }
            );
        }

    private:
        // ASYNC
        void read_header()
        {
            asio::aync_read(
                m_socket, 
                asio::buffer(&m_scratchMessage.header, sizeof(MessageHeader<MsgType>)),
                [this](std::error_code ec, std::size_t length)
                {
                    if (!ec)
                    {
                        if (m_scratchMessage.header.size > 0)
                        {
                            // get message size from header and prime asio to read body content
                            m_scratchMessage.body.resize(m_scratchMessage.header.size);
                            this->read_body();
                        }
                        // no body to read so add scratch message
                        else
                        {
                            this->add_scratch_to_incoming();
                        }
                    }
                    else
                    {
                        PLEEPLOG_ERROR("Asio error: " + ec.message());
                        // on error close socket to that server/client will cleanup
                        m_socket.close();
                    }
                }
            );
        }

        // ASYNC
        void read_body()
        {
            asio::aync_read(
                m_socket, 
                asio::buffer(&m_scratchMessage.body.data(), m_scratchMessage.body.size()),
                [this](std::error_code ec, std::size_t length)
                {
                    if (!ec)
                    {
                        // body is read, now add to queue
                        this->add_scratch_to_incoming();
                    }
                    else
                    {
                        PLEEPLOG_ERROR("Asio error: " + ec.message());
                        // on error close socket to that server/client will cleanup
                        m_socket.close();
                    }
                }
            );
        }
        
        // ASYNC
        void write_header()
        {
            asio::aync_write(
                m_socket, 
                asio::buffer(&m_outgoingMessages.front().header, sizeof(MessageHeader<MsgType>)),
                [this](std::error_code ec, std::size_t length)
                {
                    if (!ec)
                    {
                        if (m_outgoingMessages.front().body.size() > 0)
                        {
                            this->write_body();
                        }
                        // no body to write, so clear message
                        else
                        {
                            m_outgoingMessages.pop_front();

                            // prime next write
                            if (!m_outgoingMessages.empty())
                            {
                                this->write_header();
                            }
                        }
                    }
                    else
                    {
                        PLEEPLOG_ERROR("Asio error: " + ec.message());
                        // on error close socket to that server/client will cleanup
                        m_socket.close();
                    }
                }
            );
        }

        // ASYNC
        void write_body()
        {
            asio::aync_write(
                m_socket, 
                asio::buffer(&m_outgoingMessages.front().body.data(), m_outgoingMessages.front().body.size()),
                [this](std::error_code ec, std::size_t length)
                {
                    if (!ec)
                    {
                        // body is written, prime for next write
                        m_outgoingMessages.pop_front();

                        if (!m_outgoingMessages.empty())
                        {
                            this->write_header();
                        }
                    }
                    else
                    {
                        PLEEPLOG_ERROR("Asio error: " + ec.message());
                        // on error close socket to that server/client will cleanup
                        m_socket.close();
                    }
                }
            );
        }

        void add_scratch_to_incoming()
        {
            if (m_ownerType == owner::server)
            {
                m_incomingMessages.push_back({ this->shared_from_this(), m_scratchMessage });
            }
            else if (m_ownerType == owner::server)
            {
                // clients only have 1 connection, so they dont need to know shared_from_this?
                m_incomingMessages.push_back({ nullptr, m_scratchMessage });
            }

            // clear scratch?

            // prime to read next message
            this->read_header();
        }


    protected:
        // Unique asio socket per connection to some "remote"
        // TODO: is this being tcp going to cause problems?
        asio::ip::tcp::socket m_socket;

        // Shared context with asio instance used by entire client
        asio::io_context& m_asioContext;

        // Queue of Messages to be sent
        TsQueue<Message<MsgType>>       m_outgoingMessages;
        // Reference to connection OWNER's queue of Messages recieved
        // thus servers can have multiple connections all push to 1 queue
        TsQueue<OwnedMessage<MsgType>>& m_incomingMessages;
        // Storage for building incoming message
        Message<MsgType>                m_scratchMessage;

        owner m_ownerType = owner::server;
        uint32_t m_id = 0;
    };
}
}

#endif // NET_CONNECTION_H