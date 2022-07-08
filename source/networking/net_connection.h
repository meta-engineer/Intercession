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
    template<typename T_Msg>
    class Connection : public std::enable_shared_from_this<Connection<T_Msg>>
    {
    public:
        enum class owner
        {
            server,
            client
        };

        Connection(owner parent, asio::io_context& asioContext, asio::ip::tcp::socket socket, TsQueue<OwnedMessage<T_Msg>>& inQ)
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

        void connect_to_client(uint32_t id = 0)
        {
            if (m_ownerType == owner::server)
            {
                if (m_socket.is_open())
                {
                    m_id = id;

                    // prime to read
                    this->_read_header();
                }
            }
        }
        void connect_to_server(const asio::ip::tcp::resolver::results_type& endpoints)
        {
            // only clients can do this
            if (m_ownerType == owner::client)
            {
                // Attempt to connect
                asio::async_connect(m_socket, endpoints,
                    [this](std::error_code ec, asio::ip::tcp::endpoint endpoint)
                    {
                        UNREFERENCED_PARAMETER(endpoint);
                        if (!ec)
                        {
                            // now we're connected, prime to read header
                            this->_read_header();
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

        void disconnect()
        {
            if (this->is_connected())
            {
                asio::post(m_asioContext, [this]() { m_socket.close(); });
            }
        }

        bool is_connected() const
        {
            return m_socket.is_open();
        }

        void send(const Message<T_Msg>& msg)
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
                        this->_write_header();
                    }
                }
            );
        }

    private:
        // ASYNC
        void _read_header()
        {
            asio::async_read(
                m_socket, 
                asio::buffer(&m_scratchMessage.header, sizeof(MessageHeader<T_Msg>)),
                [this](std::error_code ec, std::size_t length)
                {
                    UNREFERENCED_PARAMETER(length);
                    if (!ec)
                    {
                        if (m_scratchMessage.header.size > 0)
                        {
                            // get message size from header and prime asio to read body content
                            m_scratchMessage.body.resize(m_scratchMessage.header.size);
                            this->_read_body();
                        }
                        // no body to read so add scratch message
                        else
                        {
                            this->_add_scratch_to_incoming();
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
        void _read_body()
        {
            asio::async_read(
                m_socket, 
                asio::buffer(m_scratchMessage.body.data(), m_scratchMessage.body.size()),
                [this](std::error_code ec, std::size_t length)
                {
                    UNREFERENCED_PARAMETER(length);
                    if (!ec)
                    {
                        // body is read, now add to queue
                        this->_add_scratch_to_incoming();
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
        void _write_header()
        {
            asio::async_write(
                m_socket, 
                asio::buffer(&m_outgoingMessages.front().header, sizeof(MessageHeader<T_Msg>)),
                [this](std::error_code ec, std::size_t length)
                {
                    UNREFERENCED_PARAMETER(length);
                    if (!ec)
                    {
                        if (m_outgoingMessages.front().body.size() > 0)
                        {
                            this->_write_body();
                        }
                        // no body to write, so clear message
                        else
                        {
                            m_outgoingMessages.pop_front();

                            // prime next write
                            if (!m_outgoingMessages.empty())
                            {
                                this->_write_header();
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
        void _write_body()
        {
            asio::async_write(
                m_socket, 
                asio::buffer(m_outgoingMessages.front().body.data(), m_outgoingMessages.front().body.size()),
                [this](std::error_code ec, std::size_t length)
                {
                    UNREFERENCED_PARAMETER(length);
                    if (!ec)
                    {
                        // body is written, prime for next write
                        m_outgoingMessages.pop_front();

                        if (!m_outgoingMessages.empty())
                        {
                            this->_write_header();
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

        void _add_scratch_to_incoming()
        {
            if (m_ownerType == owner::server)
            {
                m_incomingMessages.push_back({ this->shared_from_this(), m_scratchMessage });
            }
            else if (m_ownerType == owner::client)
            {
                // clients only have 1 Connection, so they dont need to know shared_from_this?
                m_incomingMessages.push_back({ nullptr, m_scratchMessage });
            }

            // clear scratch?

            // prime to read next message
            this->_read_header();
        }


    protected:
        // Unique asio socket per Connection to some "remote"
        // TODO: is this being tcp going to cause problems?
        asio::ip::tcp::socket m_socket;

        // Shared context with asio instance used by entire client
        asio::io_context& m_asioContext;

        // Queue of Messages to be sent
        TsQueue<Message<T_Msg>>       m_outgoingMessages;
        // Reference to Connection OWNER's queue of Messages recieved
        // thus servers can have multiple Connections all push to 1 queue
        TsQueue<OwnedMessage<T_Msg>>& m_incomingMessages;
        // Storage for building incoming message
        Message<T_Msg>                m_scratchMessage;

        owner m_ownerType = owner::server;
        uint32_t m_id = 0;
    };
}
}

#endif // NET_CONNECTION_H