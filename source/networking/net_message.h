#ifndef NET_MESSAGE_H
#define NET_MESSAGE_H

//#include "intercession_pch.h"
#include <vector>
#include <string>
#include <memory>

namespace pleep
{
namespace net
{
    // Defines a generic, resizable byte container to serialize/deserialize POD
    // Header is user defined type to recognise on the recieving end and deserialize
    // ALSO (at bottom) is OwnedMessage which wraps Message with a connection pointer

    // Note that MessageGeader::MsgType does not constrain the Message::DataType
    // so as standalone any MsgType can be pushed with any data.
    // I'm not sure how you would avoid that with only POD. Templating the
    // message with the struct whos data you want to send would then need that
    // struct to be converted to a integral id anyway...

    template <typename MsgType>
    struct MessageHeader
    {
        MsgType id{};
        uint32_t size = 0;
    };

    template <typename MsgType>
    struct Message
    {
        MessageHeader<MsgType> header{};
        std::vector<uint8_t> body;

        // (in bytes)
        size_t size() const
        {
            return sizeof(MessageHeader<MsgType>) + body.size();
        }

        std::string info() const
        {
            return std::string("ID: " + std::to_string(int(header.id)) + " Size: " + std::to_string(header.size));
        }

        // use stream-in operator to push data into message
        template<typename DataType>
        friend Message<MsgType>& operator<<(Message<MsgType>& msg, const DataType& data)
        {
            // only accept POD. magic modern c++ to check type of data
            if (!std::is_standard_layout<DataType>::value)
            {
                PLEEPLOG_ERROR("Data type pushed is not POD");
                throw std::runtime_error("Message operator<< could not serialize non-POD type");
            }

            // track current size of body
            uint32_t i = msg.body.size();

            // resize for data to be pushed
            // does this break the amortized exponential auto-allocating?
            // No, i think resize is ACTUALLY making elements, not just capacity
            msg.body.resize(msg.body.size() + sizeof(DataType));

            // actually physically copy the data into allocated space
            std::memcpy(msg.body.data() + i, &data, sizeof(DataType));

            // recalc message size
            msg.header.size = msg.size();

            return msg;
        }

        // use stream-out operator to pop data out of message
        template<typename DataType>
        friend Message<MsgType>& operator>>(Message<MsgType>& msg, DataType& data)
        {
            // only accept POD. magic modern c++ to check type of data
            if (!std::is_standard_layout<DataType>::value)
            {
                PLEEPLOG_ERROR("Data type popped is not POD");
                throw std::runtime_error("Message operator>> could not deserialize non-POD type");
            }

            // track index at the start of the data on "top" of the stack
            uint32_t i = msg.body.size() - sizeof(DataType);

            // actually physically copy the data into allocated space
            std::memcpy(&data, msg.body.data() + i, sizeof(DataType));

            // shrink, removing end of stack (constant time)
            msg.body.resize(i);

            // recalc message size
            msg.header.size = msg.size();
            
            return msg;
        }
    };


    // Forward declare connection owner
    template <typename MsgType>
    class Connection;

    template <typename MsgType>
    struct OwnedMessage
    {
        std::shared_ptr<Connection<MsgType>> remote = nullptr;
        Message<MsgType> msg;

        std::string info() const
        {
            // TODO: append Connection info
            return /* remote.info() + */ msg.info();
        }
    };
}
}

#endif // NET_MESSAGE_H