#ifndef MESSAGE_H
#define MESSAGE_H

//#include "intercession_pch.h"
#include <vector>
#include <string>

#include "logging/pleep_log.h"

namespace pleep
{
    // Defines a generic, resizable byte container to serialize/deserialize POD
    // Header is user defined type to recognise on the recieving end and deserialize

    // Note that MessageHeader::T_Msg does not constrain the Message::T_Data
    // so as standalone any T_Msg can be pushed with any data.
    // I'm not sure how you would avoid that with only POD. Templating the
    // message with the struct whos data you want to send would then need that
    // struct to be converted to a integral id anyway...

    template <typename T_Msg>
    struct MessageHeader
    {
        T_Msg id{};
        uint32_t size = 0;
    };

    template <typename T_Msg>
    struct Message
    {
        MessageHeader<T_Msg> header{};
        std::vector<uint8_t> body;
        
        Message() = default;
        // initialize with header id;
        Message(T_Msg id)
        {
            this->header.id = id;
        }

        // (in bytes)
        size_t size() const
        {
            return body.size();
        }

        std::string info() const
        {
            return std::string("ID: " + std::to_string(int(header.id)) + " Size: " + std::to_string(header.size));
        }
    };

    // use stream-in operator to push data into message
    template<typename T_Msg, typename T_Data>
    Message<T_Msg>& operator<<(Message<T_Msg>& msg, const T_Data& data)
    {
        // only accept POD. magic modern c++ to check type of data
        if (!std::is_standard_layout<T_Data>::value)
        {
            PLEEPLOG_ERROR("Data type pushed is not POD");
            throw std::runtime_error("Message operator<< could not serialize non-POD type");
        }

        // track current size of body
        uint32_t i = static_cast<uint32_t>(msg.size());

        // resize for data to be pushed
        // does this break the amortized exponential auto-allocating?
        // No, i think resize is ACTUALLY making elements, not just capacity
        msg.body.resize(msg.body.size() + sizeof(T_Data));

        // actually physically copy the data into allocated space
        std::memcpy(msg.body.data() + i, &data, sizeof(T_Data));

        // recalc message size
        msg.header.size = static_cast<uint32_t>(msg.size());

        return msg;
    }

    // use stream-out operator to pop data out of message
    template<typename T_Msg, typename T_Data>
    Message<T_Msg>& operator>>(Message<T_Msg>& msg, T_Data& data)
    {
        // only accept POD. magic modern c++ to check type of data
        if (!std::is_standard_layout<T_Data>::value)
        {
            PLEEPLOG_ERROR("Data type popped is not POD");
            throw std::runtime_error("Message operator>> could not deserialize non-POD type");
        }

        // cannot stream out when no data is available;
        assert(msg.size() >= sizeof(T_Data));

        // track index at the start of the data on "top" of the stack
        uint32_t i = static_cast<uint32_t>(msg.size()) - sizeof(T_Data);

        // actually physically copy the data into allocated space
        std::memcpy(&data, msg.body.data() + i, sizeof(T_Data));

        // shrink, removing end of stack (constant time)
        msg.body.resize(i);

        // recalc message size
        msg.header.size = static_cast<uint32_t>(msg.size());
        
        return msg;
    }


    // Stream operator specializations for strings

    template<typename T_Msg>
    Message<T_Msg>& operator<<(Message<T_Msg>& msg, const std::string& inString)
    {
        // we know string is non-POD type

        // track current size of body
        uint32_t i = static_cast<uint32_t>(msg.size());

        // resize for data to be pushed
        size_t inStringLength = inString.length();
        // does this break the amortized exponential auto-allocating?
        // No, i think resize is ACTUALLY making elements, not just capacity
        msg.body.resize(msg.body.size() + sizeof(size_t) + (sizeof(char) * inStringLength));

        // actually physically copy the data into allocated space
        // copy data
        std::memcpy(msg.body.data() + i, inString.data(), inStringLength);
        i += static_cast<uint32_t>(sizeof(char) * inStringLength);

        // copy size (to be extracted first)
        std::memcpy(msg.body.data() + i, &inStringLength, sizeof(size_t));

        // recalc message size
        msg.header.size = static_cast<uint32_t>(msg.size());

        return msg;
    }

    template<typename T_Msg>
    Message<T_Msg>& operator>>(Message<T_Msg>& msg, std::string& outString)
    {
        // we know string is non-POD type

        // cannot stream out when no data is available;
        assert(msg.size() >= sizeof(size_t));

        // track index at the start of the data on "top" of the stack
        uint32_t i = static_cast<uint32_t>(msg.size()) - sizeof(size_t);
        // extract the string length
        size_t outStringLength;
        std::memcpy(&outStringLength, msg.body.data() + i, sizeof(size_t));

        // now that we know the string size, move index in front of string data
        i -= static_cast<uint32_t>(sizeof(char) * outStringLength);
        // need a place to extract to (cannot use std::string data pointer)
        char* cOutString = new char[outStringLength + 1];
        // extract string data;
        std::memcpy(cOutString, msg.body.data() + i, sizeof(char) * outStringLength);
        cOutString[outStringLength] = 0;    // guarentee to be 0 terminated?
        // write to outString
        outString = cOutString;
        delete[] cOutString;

        // shrink, removing end of stack (constant time)
        msg.body.resize(i);

        // recalc message size
        msg.header.size = static_cast<uint32_t>(msg.size());

        return msg;
    }
}

#endif // MESSAGE_H