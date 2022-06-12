#include "pleep_net.h"

#include "logging/pleep_log.h"

namespace pleep
{
namespace net
{
    void test_sockets() 
    {
        // Testing
        PLEEPLOG_DEBUG("//////////////////////////");
        asio::error_code err;
        // platform ambiguous interface
        asio::io_context asioContext;
        // define connect destination
        asio::ip::tcp::endpoint endpoint(asio::ip::make_address("93.184.216.34", err), 80);
        // create socket with?from?using? context
        asio::ip::tcp::socket socket(asioContext);

        // use socket to try to connect
        socket.connect(endpoint, err);

        if (!err)
        {
            PLEEPLOG_DEBUG("asio connected");
        }
        else
        {
            PLEEPLOG_DEBUG("asio failed to connect: " + err.message());
        }

        // use connected socket
        if (socket.is_open())
        {
            std::string requestHTML =
                "GET /index.hdml HTTP/1.1\r\n"
                "Host: example.com\r\n"
                "Connection: close\r\n\r\n";

                socket.write_some(asio::buffer(requestHTML.data(), requestHTML.size()), err);

                socket.wait(socket.wait_read);

                uint32_t numBytes = socket.available();
                //PLEEPLOG_DEBUG("Bytes available: " + std::to_string(numBytes));

                if (numBytes > 0)
                {
                    std::vector<char> responseBuffer(numBytes);
                    socket.read_some(asio::buffer(responseBuffer.data(), responseBuffer.size()), err);

                    std::string responseString;
                    for (char c : responseBuffer)
                        responseString += c;
                    PLEEPLOG_DEBUG("HTTP response:\n" + responseString);
                }
        }

        // immediately stop whole context for testing
        // TODO: create general quit event?
        //m_sharedBroker->send_event(events::window::QUIT);
        PLEEPLOG_DEBUG("//////////////////////////");
        
    }
}
}