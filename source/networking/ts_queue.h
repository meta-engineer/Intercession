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
    template<typename ElementType>
    class TsQueue
    {
    public:
        TsQueue() = default;
        // not safe to have copies
        TsQueue(const TsQueue<ElementType>&) = delete;
        // I don't know why this was suggested to be virtual
        virtual ~TsQueue() { clear(); }
        
        // Wrap deque methods

        const ElementType& front()
        {
            const std::lock_guard<std::mutex> lk(m_dequeMux);
            return m_deque.front();
        }
        const ElementType& back()
        {
            const std::lock_guard<std::mutex> lk(m_dequeMux);
            return m_deque.back();
        }
        
        void push_back(const ElementType& item)
        {
            const std::lock_guard<std::mutex> lk(m_dequeMux);
            m_deque.emplace_back(std::move(item));
        }
        void push_front(const ElementType& item)
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

        ElementType pop_front()
        {
            const std::lock_guard<std::mutex> lk(m_dequeMux);
            const ElementType& item = std::move(m_deque.front());
            m_deque.pop_front();
            return item;
        }
        ElementType pop_back()
        {
            const std::lock_guard<std::mutex> lk(m_dequeMux);
            const ElementType& item = std::move(m_deque.back());
            m_deque.pop_back();
            return item;
        }

    protected:
        // cannot use scoped_lock without cxx17, so "deprecated" lock_guard<std::mutex> can substitute
        std::mutex m_dequeMux;
        std::deque<ElementType> m_deque;
    };
}

#endif // TS_QUEUE_H