#ifndef TS_BREAKPOINT_QUEUE_H
#define TS_BREAKPOINT_QUEUE_H

//#include "intercession_pch.h"
#include <mutex>
#include <list>
#include <utility>

namespace pleep
{
    // ThreadSafe Queue, similar to TsDeque but is unidiractional and provides functionality
    //   to set a breakpoint in the queue for sequentially iterating (from front to back)
    //   and prevent other threads from popping data until it passes the breakpoint.
    // Removing the breakpoint (default state) makes the queue act as a normal threadsafe queue
    template<typename T_Element>
    class TsBreakpointQueue
    {
    public:
        TsBreakpointQueue()
        {
            // make iterator relavant to m_list
            m_breakpoint = m_list.end();
            m_isBreakpointActive = false;
        }
        // not safe to have copies? m_listMux is not copyable
        TsBreakpointQueue(const TsBreakpointQueue<T_Element>&) = delete;
        // virtual for some reason?
        virtual ~TsBreakpointQueue() { clear(); }

        // Wrap "queue" methods

        // quickly return const reference for checking content
        const T_Element& peek_front()
        {
            const std::lock_guard<std::mutex> lk(m_listMux);
            if (m_breakpoint == m_list.begin()) // if list is empty this will also be true
            {
                throw std::range_error("Cannot peek front on TsBreakpointQueue with nothing available.");
            }

            return m_list.front();
        }

        // you can always push without fail
        void push_back(const T_Element& item)
        {
            const std::lock_guard<std::mutex> lk(m_listMux);
            // always allow pushing regardless of breakpoint
            m_list.emplace_back(std::move(item));
            
            // but if breakpoint is at end (while active), then move it back to point to new data
            if (m_isBreakpointActive && m_breakpoint == m_list.end()) m_breakpoint--;

            // signal any waiters
            m_waitCv.notify_one();
        }
        // returns false if breakpoint is active and at front
        bool pop_front(T_Element& dest)
        {
            const std::lock_guard<std::mutex> lk(m_listMux);
            if (m_list.empty()) return false;

            // If breakpoint is at beginning then we should not allow normal popping, so that parallel will always win the race condition
            if (m_isBreakpointActive && m_breakpoint == m_list.begin()) return false;

            dest = std::move(m_list.front());
            m_list.pop_front();

            return true;
        }
        
        bool empty()
        {
            const std::lock_guard<std::mutex> lk(m_listMux);
            return m_list.empty();
        }
        size_t count()
        {
            const std::lock_guard<std::mutex> lk(m_listMux);
            return m_list.size();
        }
        void clear()
        {
            const std::lock_guard<std::mutex> lk(m_listMux);
            m_list.clear();
            // breakpoint is definately invalidated, but could still be active?
            m_breakpoint = m_list.end();
        }

        // use this to block until list is pushed-to by another thread
        void wait_for_data()
        {
            const std::lock_guard<std::mutex> lk(m_listMux);
            while (this->empty() && m_breakpoint != m_list.begin())
            {
                m_waitCv.wait(waitLk);
            }
        }

        // "breakpoint" methods

        void set_breakpoint_at_begin()
        {
            const std::lock_guard<std::mutex> lk(m_listMux);
            m_breakpoint = m_list.begin();
            m_isBreakpointActive = true;
        }
        void remove_breakpoint()
        {
            const std::lock_guard<std::mutex> lk(m_listMux);
            m_breakpoint = m_list.end();
            m_isBreakpointActive = false;
        }

        // quickly return const reference for checking content
        const T_Element& peek_breakpoint()
        {
            const std::lock_guard<std::mutex> lk(m_listMux);
            if (m_breakpoint == m_list.end()) // if list is empty this will also be true
            {
                throw std::range_error("Cannot peek breakpoint on TsBreakpointQueue with nothing available.");
            }

            return *m_breakpoint;
        }
        // insert element at breakpoint position less than it (towards-front),
        // breakpoint stays at some value as before push (effectively moving towards-back 1 index)
        // returns false if breakpoint is not active
        bool push_at_breakpoint(const T_Element& item)
        {
            const std::lock_guard<std::mutex> lk(m_listMux);
            if (!m_isBreakpointActive) return false;

            m_list.insert(m_breakpoint, item);
            
            // signal any waiters
            m_waitCv.notify_one();

            // breakpoint will remain at same value as before insert
            
            return true;
        }

        // return element at breakpoint position in dest,
        // move breakpoint towards back 1 position (to be at neighbor to the popped element)
        // returns false if list is empty or there is no breakpoint or if breakpoint is past the end
        bool pop_at_breakpoint(T_Element& dest)
        {
            const std::lock_guard<std::mutex> lk(m_listMux);
            if (m_list.empty()) return false;
            if (!m_isBreakpointActive) return false;
            if (m_breakpoint == m_list.end()) return false;

            dest = *m_breakpoint;

            m_breakpoint = m_list.erase(m_breakpoint);
            // erase returns iterator following the removed element (towards-back)

            return true;
        }

        // convenience function to check if peeking/popping is valid
        bool is_data_available()
        {
            const std::lock_guard<std::mutex> lk(m_listMux);
            if (m_list.empty() || m_breakpoint == m_list.begin()) return false;

            return true;
        }
        bool is_data_available_at_breakpoint()
        {
            const std::lock_guard<std::mutex> lk(m_listMux);
            // if list is empty begin == end
            // if breakpoint disabled it will be set to end
            // if breakpoint active, but just at end, then you can't peek/pop
            if (m_breakpoint == m_list.end()) return false;

            return true;
        }

    protected:
        // cannot use scoped_lock without cxx17, so "deprecated" lock_guard<std::mutex> can substitute
        std::mutex m_listMux;
        // secretly use a list (for breakpoint iterators)
        std::list<T_Element> m_list;

        std::condition_variable m_waitCv;
        
        // iterator for constant time access to middle of "queue"
        typename std::list<T_Element>::iterator m_breakpoint;
        // true implies breakpoint should limit pop_front, and allow breakpoint access methods
        bool m_isBreakpointActive;
    };
}

#endif // TS_BREAKPOINT_QUEUE_H