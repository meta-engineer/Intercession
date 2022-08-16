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
#include "networking/net_i_server.h"
#include "networking/pleep_crypto.h"

#define MAX_MESSAGE_BODY_BYTES 256

namespace pleep
{
namespace net
{
    // Forward declare pointer
    // How do we access methods with only a foreward declaration? templates?
    template<typename T_Msg>
    class I_Server;

    // For outgoing (client) connections: Fresh socket is given to Connection and connect_to_remote establishes communication with remote endpoint.
    //     Internally: connect_to_remote() -> start_communication() -> _riposte_validation() -> _read_header() (and loop indefinately on _read_header() -> (optionally _read_body()) -> _add_scratch_to_incoming() -> repeat)
    //
    // For incoming (server) connections, a pre-established (asio acceptor) socket is given and start_communication(server) validates communication with the accepted remote endpoint.
    //     Internally: start_communication() -> _initiate_validation() -> _reprise_validation -> _read_header() (and loop indefinately on _read_header() -> (optionally _read_body()) -> _add_scratch_to_incoming() -> repeat)
    //
    // Both connections can send() message data to write data across the connection
    //     Internally: send() -> _write_header() -> (optionally _write_body())
    template<typename T_Msg>
    class Connection : public std::enable_shared_from_this<Connection<T_Msg>>
    {
    public:
        Connection(asio::io_context& asioContext, asio::ip::tcp::socket socket, TsQueue<OwnedMessage<T_Msg>>& inQ)
            : m_asioContext(asioContext)
            , m_socket(std::move(socket))
            , m_incomingMessages(inQ)
        {
        }
        ~Connection()
        {}

        void set_id(uint32_t id)
        {
            m_id = id;
        }
        uint32_t get_id() const
        {
            return m_id;
        }

        // callback_server truthiness determines if this connection should "lead" (non-null) or "follow"(null) in protocol
        void start_communication(I_Server<T_Msg>* callback_server = nullptr)
        {
            if (m_socket.is_open())
            {
                if (callback_server != nullptr)
                {
                    // validation chain will use callback's on_remote_validated,
                    // and start _read_header() on success
                    this->_initiate_validation(callback_server);
                }
                else // (no callback_server)
                {
                    // validation chain will start _read_header() on success
                    this->_riposte_validation();
                }
            }
            else
            {
                PLEEPLOG_WARN("Called start_communication on Connection with closed socket.");
            }
        }
        // only "clients" should use this
        // calls start_communication implicitly after connection is made
        void connect_to_remote(const asio::ip::tcp::resolver::results_type& endpoints)
        {
            if (m_socket.is_open())
            {
                PLEEPLOG_WARN("Cannot establish new connection with an in-use socket. disconnect() first.");
                return;
            }
            
            // Attempt to connect
            asio::async_connect(m_socket, endpoints,
                [this](std::error_code ec, asio::ip::tcp::endpoint endpoint)
                {
                    UNREFERENCED_PARAMETER(endpoint);
                    if (!ec)
                    {
                        this->start_communication();
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

        void disconnect()
        {
            if (this->is_connected())
            {
                asio::post(m_asioContext, [this]() { m_socket.close(); });
            }
        }

        // report socket state
        bool is_connected() const
        {
            return m_socket.is_open();
        }

        // report if connection is connected, initialized, and safe to send across
        bool is_ready() const
        {
            return this->is_connected() && m_handshakeSuccess;
        }

        asio::ip::tcp::endpoint get_endpoint()
        {
            return m_socket.remote_endpoint();
        }

        // Append msg into to-write message queue and proceed to _write_header
        // does not return if message was not sent (or failed during async methods)
        void send(const Message<T_Msg>& msg)
        {
            if (!this->is_ready())
            {
                PLEEPLOG_WARN("Cannot send message, Connection is not ready (yet?).");
                return;
            }
            // send a "job" to the context
            asio::post(
                m_asioContext,
                [this, msg]()
                {
                    // if new size > 1, messages were already being written
                    size_t newSize = m_outgoingMessages.push_back(msg);

                    // prime for writing if not already
                    if (newSize == 1)
                    {
                        this->_write_header();
                    }
                }
            );
        }

    private:
        ////////////////////////////// READ MESSAGES //////////////////////////////

        // ASYNC - start task to read fixed header.
        // fill m_scratchMessage with type and body size
        // proceed to _read_body if header indicates non-zero body size
        // or proceed to _add_scratch_to_incoming with body-less message
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
                        if (m_scratchMessage.header.size > MAX_MESSAGE_BODY_BYTES)
                        {
                            PLEEPLOG_WARN("Message header size " + std::to_string(m_scratchMessage.header.size) + ", greater than max " + std::to_string(MAX_MESSAGE_BODY_BYTES) + ". Ignoring...");
                            this->_read_header();
                        }
                        else if (m_scratchMessage.header.size > 0)
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

        // ASYNC - start task to read message body
        // dynamic size passed through m_scratchMessage
        // proceed to _add_scratch_to_incoming
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

        // push finished "read" message to queue available for app
        // "recurse" to _read_header
        void _add_scratch_to_incoming()
        {
            // clients only have 1 Connection, so they dont need to know shared_from_this,
            // but to keep behaviour ambiguous to client or server we have to store it for server
            // does maintaining all the shared counts unnecessarily cost too much?
            m_incomingMessages.push_back({ this->shared_from_this(), m_scratchMessage });

            // clear scratch?
            // body will be resized (potentially to 0) on next header read

            // prime to read next message
            this->_read_header();
        }
        
        ////////////////////////////// WRITE MESSAGES //////////////////////////////

        // ASYNC - start task to write message header from front of to-write message queue
        // proceed to _write_body if non-zero message body
        // or pop message and "recurse" to _write_header if no message body AND to-write message queue is non-empty
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
                            std::pair<Message<T_Msg>,size_t> popped = m_outgoingMessages.pop_front();

                            // prime next write
                            if (popped.second > 0)
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

        // ASYNC - start task to write message body from  front of to-write message queue
        // pop message and "recurse" to _write_header if to-write message queue is non-empty
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
                        std::pair<Message<T_Msg>,size_t> popped = m_outgoingMessages.pop_front();

                        if (popped.second > 0)
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

        ////////////////////////////// VALIDATION //////////////////////////////
        // Validation steps (attempt) to check cryptographic integrity (at connection level)
        //   of the client as a first pass, and forcibly decline bad actors 
        // App level should do additional validation to check for matching versions/protocols
        //   and mutually disconect upon improper communication

        // ASYNC - start task to send validation data
        // proceed to _reprise_validation()
        void _initiate_validation(I_Server<T_Msg>* callback_server)
        {
            assert(callback_server != nullptr);

            // get "random" data to use
            m_handshakePlaintext = uint32_t(std::chrono::system_clock::now().time_since_epoch().count());

            asio::async_write(
                m_socket, 
                asio::buffer(&m_handshakePlaintext, sizeof(uint32_t)),
                [this, callback_server](std::error_code ec, std::size_t length)
                {
                    UNREFERENCED_PARAMETER(length);
                    if (!ec)
                    {
                        this->_reprise_validation(callback_server); 
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
        // ASYNC - start task to wait for remote's data from _initiate_validation()
        // respond and proceed to _read_header() loop
        void _riposte_validation()
        {

            asio::async_read(
                m_socket, 
                asio::buffer(&m_handshakePlaintext, sizeof(uint32_t)),
                [this](std::error_code ec, std::size_t length)
                {
                    UNREFERENCED_PARAMETER(length);
                    if (!ec)
                    {
                        // compute checksum
                        m_handshakeChecksum = pleep::Squirrel3(m_handshakePlaintext);

                        // write back to server
                        asio::async_write(
                            m_socket, 
                            asio::buffer(&m_handshakeChecksum, sizeof(uint32_t)),
                            [this](std::error_code ec, std::size_t length)
                            {
                                UNREFERENCED_PARAMETER(length);
                                if (!ec)
                                {
                                    // after sending validation, wait for app level messages
                                    // (read header loop)
                                    this->_read_header();
                                    m_handshakeSuccess = true;
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
                    else
                    {
                        PLEEPLOG_ERROR("Asio error: " + ec.message());
                        // on error close socket to that server/client will cleanup
                        m_socket.close();
                    }
                }
            );
        }
        // ASYNC - start task to wait for remote's checksum from _riposte_validation()
        // send acknowledge and proceed to _read_header loop
        void _reprise_validation(I_Server<T_Msg>* callback_server)
        {
            assert(callback_server != nullptr);

            asio::async_read(
                m_socket, 
                asio::buffer(&m_handshakeChecksum, sizeof(uint32_t)),
                [this, callback_server](std::error_code ec, std::size_t length)
                {
                    UNREFERENCED_PARAMETER(length);
                    if (!ec)
                    {
                        uint32_t correctChecksum = pleep::Squirrel3(m_handshakePlaintext);
                        if (correctChecksum == m_handshakeChecksum)
                        {
                            m_handshakeSuccess = true;

                            callback_server->on_remote_validated(this->shared_from_this());
                            // app level can now send version/config data and client can
                            // voulentarily disconnect from mismatch

                            // go to read header loop
                            this->_read_header();
                        }
                        else
                        {
                            // failed validation
                            PLEEPLOG_TRACE("Connected client failed validation");
                            m_socket.close();
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

        // unique id to distinguish multiple connections used by a server
        uint32_t m_id = 0;

        // Basic (application level) handshake validation
        uint32_t m_handshakePlaintext = 0;
        uint32_t m_handshakeChecksum = 0;
        bool m_handshakeSuccess = false;
    };
}
}

#endif // NET_CONNECTION_H