#ifndef TS_QUEUE_H
#define TS_QUEUE_H

//#include "intercession_pch.h"
#include <mutex>
#include <deque>

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
        
        void push_back(const T_Element& item)
        {
            const std::lock_guard<std::mutex> lk(m_dequeMux);
            m_deque.emplace_back(std::move(item));
        }
        void push_front(const T_Element& item)
        {
            const std::lock_guard<std::mutex> lk(m_dequeMux);
            m_deque.emplace_front(std::move(item));
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

        T_Element pop_front()
        {
            const std::lock_guard<std::mutex> lk(m_dequeMux);
            const T_Element item = std::move(m_deque.front());
            m_deque.pop_front();
            return item;
        }
        T_Element pop_back()
        {
            const std::lock_guard<std::mutex> lk(m_dequeMux);
            const T_Element item = std::move(m_deque.back());
            m_deque.pop_back();
            return item;
        }

    protected:
        // cannot use scoped_lock without cxx17, so "deprecated" lock_guard<std::mutex> can substitute
        std::mutex m_dequeMux;
        std::deque<T_Element> m_deque;
    };
}

#endif // TS_QUEUE_H