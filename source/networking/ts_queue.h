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

        // directly access dq front
        // Cannot be exception safe because you cannot return pass by reference without moving the value
        // MUST catch exception in case deque is empty
        const T_Element& front()
        {
            const std::lock_guard<std::mutex> lk(m_dequeMux);
            if (m_deque.empty())
            {
                throw std::range_error("Called front() on empty TsQueue");
            }
            return m_deque.front();
        }
        // directly access dq back
        // Cannot be exception safe because you cannot return pass by reference without moving the value
        // MUST catch exception in case deque is empty
        const T_Element& back()
        {
            const std::lock_guard<std::mutex> lk(m_dequeMux);
            if (m_deque.empty())
            {
                throw std::range_error("Called back() on empty TsQueue");
            }
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

        // return false if deque is empty
        // otherwise move popped element to reference and return true
        bool pop_front(T_Element& dest)
        {
            const std::lock_guard<std::mutex> lk(m_dequeMux);
            if (m_deque.empty()) return false;
            dest = std::move(m_deque.front());
            m_deque.pop_front();
            return true;
        }
        // return popped element and remaining size of queue
        bool pop_back(T_Element& dest)
        {
            const std::lock_guard<std::mutex> lk(m_dequeMux);
            if (m_deque.empty()) return false;
            dest = std::move(m_deque.back());
            m_deque.pop_back();
            return true;
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