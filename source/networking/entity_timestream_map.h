#ifndef ENTITY_TIMESTREAM_MAP_H
#define ENTITY_TIMESTREAM_MAP_H

//#include "intercession_pch.h"
//#include <deque>
#include <unordered_map>
#include <mutex>
#include <memory>

#include "networking/ts_breakpoint_queue.h"
#include "networking/net_message.h"
#include "ecs/ecs_types.h"
#include "events/event_types.h"

namespace pleep
{
    // Track entities and components through time
    // Every Network tick will update data locally as well as update
    // a set of past timestreams for each entity, shared with child server

    // each timestream should be associated to one entity
    // they should contain a set of timestamped entity update messages 
    // ORDERED by timestamp. It needs to be back pushable and front poppable.
    // Also, messages beyond the front need to be interrogatable (for latency compensation)
    // https://www.reddit.com/r/Overwatch/comments/3u5kfg/everything_you_need_to_know_about_tick_rate/
    // They do NOT need to be randomly indexable (exact timestamp will not be knowable)
    // But may need to be generally searchable. 
    //   EX: the first message after time X (upper_bound?)
    // (if they are maintained in order we can search in logarithmic time)
    // Does it need to be Thread Safe? if we need iterators then that might not be possible

    // A Deque allows constant push and pop per network tick, but proper order of timestamps 
    // will have to be manually maintained. (inserting out of order would be logarithmic and
    // have to be manually written)

    // A map allows messages to be automatically sorted by timestamp, 
    // but pushing and popping is logarithmic... cringe

    // TODO: we may want to limit usage depending on if you are the sender or reciever?
    // we could have seperate instances which share timestreams and accept a bool
    // to restrict methods

    class EntityTimestreamMap
    {
    public:
        // Timestream is a queue accessed by future (push_back), past (pop_front)
        //   as well as paralle who needs access to the middle, so use a TsBreakpointQueue
        //   to allow the use of iterators without any multithread nonsense
        // Hard-code message type to use EventId (to avoid template cascading)
        using Timestream = TsBreakpointQueue<EventMessage>;

        EntityTimestreamMap() = default;

        // Wrap message Queue methods to maintain timestamp ordering.

        // If no stream exists emplace it.
        // No strict ordering of messages enforced, msg coherency will be use for pop availability
        // coherency is circular so there is no catch-all default value
        void push_to_timestream(Entity entity, const EventMessage& msg)
        {
            const std::lock_guard<std::mutex> lk(m_mapMux);
            
            if (m_areBreakpointsActive && m_timestreams.count(entity) == 0)
            {
                // create and set breakpoint for new timestream
                m_timestreams[entity].set_breakpoint_at_begin();
            }

            // TODO: detect when timstreams has gone way beyond expected capacity and stop pushing
            //PLEEPLOG_DEBUG("Pushing event: " + std::to_string(msg.header.id) + " for entity: " + std::to_string(entity) + " at coherency: " + std::to_string(msg.header.coherency));

            // operator[] emplaces with default constructor for TsBreakpointQueue
            m_timestreams[entity].push_back(msg);
        }
        void push_to_timestream_at_breakpoint(Entity entity, const EventMessage& msg)
        {
            const std::lock_guard<std::mutex> lk(m_mapMux);

            if (m_areBreakpointsActive && m_timestreams.count(entity) == 0)
            {
                // create and set breakpoint for new timestream
                m_timestreams[entity].set_breakpoint_at_begin();
            }

            // operator[] emplaces with default constructor for TsBreakpointQueue
            if (m_timestreams[entity].push_at_breakpoint(msg) == false)
            {
                PLEEPLOG_CRITICAL("BREAKPOINT FAILED?!");
            }
        }

        // check if timestream exists for an entity, if it is non-empty,
        // AND if the front value has a coherency <= currentCoherency
        bool entity_has_data(Entity entity, uint16_t currentCoherency)
        {
            const std::lock_guard<std::mutex> lk(m_mapMux);

            return is_data_available(entity, currentCoherency);
        }
        // check if breakpoint is active AND if it has any data behind it
        bool entity_has_data_at_breakpoint(Entity entity, uint16_t currentCoherency)
        {
            const std::lock_guard<std::mutex> lk(m_mapMux);

            return is_data_available_at_breakpoint(entity, currentCoherency);
        }

        // check if entity has an active timestream (empty or not)
        bool entity_has_timestream(Entity entity)
        {
            return m_timestreams.count(entity);
        }

        // Return all Entities which exist in the timestream
        std::vector<Entity> get_entities_with_streams()
        {
            std::vector<Entity> entities;

            const std::lock_guard<std::mutex> lk(m_mapMux);

            for (auto const& timestream_it : m_timestreams)
            {
                entities.push_back(timestream_it.first);
            }

            return entities;
        }

        // we should only be able to pop once the correct coherency has been reached
        bool pop_from_timestream(Entity entity, uint16_t currentCoherency, EventMessage& dest)
        {
            const std::lock_guard<std::mutex> lk(m_mapMux);

            // check if data is available internally to avoid lock juggling
            if (!is_data_available(entity, currentCoherency)) return false;
            //PLEEPLOG_DEBUG("Popping for entity: " + std::to_string(entity) + " on coherency: " + std::to_string(currentCoherency) + ". There are " + std::to_string(m_timestreams.at(entity).count()) + " messages.");

            // breakpoint may still prevent the "avaiable" data from being popped
            return m_timestreams.at(entity).pop_front(dest);
        }
        bool pop_from_timestream_at_breakpoint(Entity entity, uint16_t currentCoherency, EventMessage& dest)
        {
            const std::lock_guard<std::mutex> lk(m_mapMux);
            
            // check if data is available internally to avoid lock juggling
            if (!is_data_available_at_breakpoint(entity, currentCoherency)) return false;

            return m_timestreams.at(entity).pop_at_breakpoint(dest);
        }

        // Clear timestream for specified Entity
        void clear(Entity entity)
        {
            const std::lock_guard<std::mutex> lk(m_mapMux);
            if (m_timestreams.count(entity)) m_timestreams.at(entity).clear();  // queue.clear()
        }
        // Clear ALL timestreams
        void clear()
        {
            const std::lock_guard<std::mutex> lk(m_mapMux);
            for (auto& timestream_it : m_timestreams)
            {
                timestream_it.second.clear();   // queue.clear()
            }
        }

        // Remove indexed timestream for specified Entity
        void remove(Entity entity)
        {
            const std::lock_guard<std::mutex> lk(m_mapMux);
            m_timestreams.erase(entity);
        }
        // Remove ALL indexed timestreams
        void remove()
        {
            const std::lock_guard<std::mutex> lk(m_mapMux);
            m_timestreams.clear();              // unordered_map.clear()
        }

        // ativate all mapped Timestream breakpoints
        void set_breakpoints()
        {
            const std::lock_guard<std::mutex> lk(m_mapMux);
            for (auto& timestreamIt : m_timestreams)
            {
                timestreamIt.second.set_breakpoint_at_begin();
            }
            m_areBreakpointsActive = true;
        }
        // remove all mapped Timestream breakpoints
        void remove_breakpoints()
        {
            const std::lock_guard<std::mutex> lk(m_mapMux);
            for (auto& timestreamIt : m_timestreams)
            {
                timestreamIt.second.remove_breakpoint();
            }
            m_areBreakpointsActive = false;
        }

    private:
        // check if timestream exists for an entity, if it is non-empty,
        // and if the front value has a coherency <= currentCoherency
        // without holding the lock
        bool is_data_available(Entity entity, uint16_t currentCoherency)
        {
            // no lock, only for internal use
            auto timestreams_it = m_timestreams.find(entity);
            return timestreams_it != m_timestreams.end()
                && timestreams_it->second.is_data_available()
                && coherency_greater_or_equal(currentCoherency, timestreams_it->second.peek_front().header.coherency);
        }
        bool is_data_available_at_breakpoint(Entity entity, uint16_t currentCoherency)
        {
            // no lock, only for internal use
            auto timestreams_it = m_timestreams.find(entity);
            return timestreams_it != m_timestreams.end()
                && timestreams_it->second.is_data_available_at_breakpoint()
                && coherency_greater_or_equal(currentCoherency, timestreams_it->second.peek_breakpoint().header.coherency);
        }

        // check if timestream exists

        std::unordered_map<Entity, Timestream> m_timestreams;

        std::mutex m_mapMux;

        // ensure newly emplaces timestreams get breakpoints
        bool m_areBreakpointsActive = false;
    };
}

#endif // ENTITY_TIMESTREAM_MAP_H