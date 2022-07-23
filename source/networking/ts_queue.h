#ifndef TS_QUEUE_H
#define TS_QUEUE_H

//#include "intercession_pch.h"
#include <mutex>
#include <deque>
#include <utility>

namespace pleep
{
    // "Thread Safe Queue", while not directly related to networking
    // (due to templating) This queue will buffer incoming/outgoing
    // network messages on an app's connection, which will by async
    template<typename T_Element>
    class TsQueue
    {
    public:
        TsQueue() = default;
        // not safe to have copies
        TsQueue(const TsQueue<T_Element>&) = delete;
        // I don't know why this was suggested to be virtual
        virtual ~TsQueue() { clear(); }
        
        // Wrap deque methods

        const T_Element& front()
        {
            const std::lock_guard<std::mutex> lk(m_dequeMux);
            return m_deque.front();
        }
        const T_Element& back()
        {
            const std::lock_guard<std::mutex> lk(m_dequeMux);
            return m_deque.back();
        }
        
        // returns size including item after atomic push
        size_t push_back(const T_Element& item)
        {
            const std::lock_guard<std::mutex> lk(m_dequeMux);
            m_deque.emplace_back(std::move(item));
            
            // signal any waiters
            m_waitCv.notify_one();
            return m_deque.size();
        }
        // returns size including item after atomic push
        size_t push_front(const T_Element& item)
        {
            const std::lock_guard<std::mutex> lk(m_dequeMux);
            m_deque.emplace_front(std::move(item));

            // signal any waiters
            m_waitCv.notify_one();
            return m_deque.size();
        }

        bool empty()
        {
            const std::lock_guard<std::mutex> lk(m_dequeMux);
            return m_deque.empty();
        }
        size_t count()
        {
            const std::lock_guard<std::mutex> lk(m_dequeMux);
            return m_deque.size();
        }
        void clear()
        {
            const std::lock_guard<std::mutex> lk(m_dequeMux);
            m_deque.clear();
        }

        // return popped element and remaining size of queue
        std::pair<T_Element,size_t> pop_front()
        {
            const std::lock_guard<std::mutex> lk(m_dequeMux);
            const T_Element item = std::move(m_deque.front());
            m_deque.pop_front();
            return {item, m_deque.size()};
        }
        // return popped element and remaining size of queue
        std::pair<T_Element,size_t> pop_back()
        {
            const std::lock_guard<std::mutex> lk(m_dequeMux);
            const T_Element item = std::move(m_deque.back());
            m_deque.pop_back();
            return {item, m_deque.size()};
        }

        // use this to block until incoming message queue is populated by async connections
        // instead of polling with I_Server::process_received_messages()
        void wait_for_data()
        {
            const std::lock_guard<std::mutex> lk(m_dequeMux);
            while (this->empty())
            {
                m_waitCv.wait(waitLk);
            }
        }

    protected:
        // cannot use scoped_lock without cxx17, so "deprecated" lock_guard<std::mutex> can substitute
        std::mutex m_dequeMux;
        std::deque<T_Element> m_deque;

        std::condition_variable m_waitCv;
    };
}

#endif // TS_QUEUE_H