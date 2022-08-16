#ifndef NET_MESSAGE_H
#define NET_MESSAGE_H

//#include "intercession_pch.h"
#include <memory>
#include <string>

#include "events/message.h"

namespace pleep
{
namespace net
{
    // Borrow event message serialization to package network messages

    // Forward declare connection owner
    template <typename T_Msg>
    class Connection;

    // Wraps Message with a connection pointer
    template <typename T_Msg>
    struct OwnedMessage
    {
        std::shared_ptr<Connection<T_Msg>> remote = nullptr;
        Message<T_Msg> msg;

        std::string info() const
        {
            return "[" + std::to_string(remote.get_id()) + "] " + msg.info();
        }
    };
}

    // Wraps Message with a chrono time_point
    template <typename T_Msg>
    struct TimestampedMessage
    {
        std::chrono::system_clock::time_point timeVal;// = std::chrono::system_clock::now();
        Message<T_Msg> msg;
        
        std::string info() const
        {
            return "(" + std::to_string(timeVal.count()) + ") " + msg.info();
        }
    };
}

#endif // NET_MESSAGE_H