#ifndef MESSAGE_CONDUIT_H
#define MESSAGE_CONDUIT_H

//#include "intercession_pch.h"
#include <utility>
#include <memory>

#include "networking/ts_queue.h"
#include "events/message.h"

namespace pleep
{
    // Since Timeslices must communicate TemporalEntity CausalChainLink count between ANY
    // two Timeslices, it may be redundant to have shared memory message transfer
    // given we already have network connections.
    // However we could segregate client-server and server-server communication
    // as network connections and shared memory connections
    // then the handlers could be seperated (and use the same Event types differently)
    // Each Timeslice would need 1 incoming queue and X-1 outgoing queues (for X total timeslices)
    // and ideally map each queue based on its known timesliceId

    // ... so we don't want a one way message conduit, we want a multiplex.
    // reading from the multiplex would access our own queue
    // and writing to the multiplex would require passing a timesliceId

    // "local" shared memory connection for parent/child servers
    // MessageConduits need to come in pairs for parent/child
    // where the incoming and outgoing message queues are swapped
    template<typename T_Msg>
    class MessageConduit
    {
    public:
        MessageConduit(std::shared_ptr<TsQueue<Message<T_Msg>>> incomingQueue, std::shared_ptr<TsQueue<Message<T_Msg>>> outgoingQueue)
            : m_incomingMessages(incomingQueue)
            , m_outgoingMessages(outgoingQueue)
        {}
        ~MessageConduit();

        // restrict access to outgoing queue to only send
        void send_message(Message<T_Msg>& newMsg)
        {
            m_outgoingMessages->push_back(newMsg);
        }

        // restrict access to incoming queue to only receive
        size_t is_incoming_messages_empty()
        {
            return m_incomingMessages->empty();
        }

        // has same exception behaviour of deque on popping empty deque
        Message<T_Msg> pop_next_message()
        {
            return m_incomingMessages->pop_front();
        }

        // Do conduits need intercession splicing?

    private:
        // pointers to queues shared with this Conduit's pair
        std::shared_ptr<TsQueue<Message<T_Msg>>> m_incomingMessages;
        std::shared_ptr<TsQueue<Message<T_Msg>>> m_outgoingMessages;
    };

    template<typename T_Msg>
    std::pair<MessageConduit<T_Msg>,MessageConduit<T_Msg>> generate_conduit_pair()
    {
        auto queueA = std::make_shared<TsQueue<Message<T_Msg>>>();
        auto queueB = std::make_shared<TsQueue<Message<T_Msg>>>();
        // is returning non-heap allocated conduits ok? 
        // the queue pointers should get copied and remain alive
        return std::make_pair(MessageConduit(queueA, queueB), MessageConduit(queueB, queueA));
    }
}

#endif // MESSAGE_CONDUIT_H