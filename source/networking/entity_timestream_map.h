#ifndef ENTITY_TIMESTREAM_MAP_H
#define ENTITY_TIMESTREAM_MAP_H

//#include "intercession_pch.h"
//#include <deque>
#include <unordered_map>
#include <mutex>
#include <memory>

#include "networking/ts_queue.h"
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
        // Timestream could be it's own class to manage Message ordering
        // do we need use of deque iterators? or do we need threadsafe?
        // we could provide specific extra threadsafe operations like searches to maintain threadsafety
        // Hard-code message type to use EventId (to avoid template cascading)
        using Timestream = TsQueue<Message<EventId>>;

        EntityTimestreamMap() = default;

        // deadlock-avoiding copy between timestreams
        static void link_timestreams(std::shared_ptr<EntityTimestreamMap> source,
                                     std::shared_ptr<EntityTimestreamMap> destination)
        {
            if (source == nullptr || destination == nullptr)
            {
                PLEEPLOG_WARN("Called link_timestreams on null timestream map pointers?!");
                return;
            }

            // necessary?
            PLEEPLOG_DEBUG("unlink source timestream");
            EntityTimestreamMap::unlink_timestreams(source);
            PLEEPLOG_DEBUG("unlink destination timestream");
            EntityTimestreamMap::unlink_timestreams(destination);
            destination->clear();

            static std::mutex deadlockMux;
            // since only 1 thread can have this lock...
            const std::lock_guard<std::mutex> lk(deadlockMux);
            // only 1 thread should ever be trying to hold BOTH locks at once
            const std::lock_guard<std::mutex> sk(source->m_mapMux);
            const std::lock_guard<std::mutex> dk(destination->m_mapMux);

            PLEEPLOG_DEBUG("squired both locks");
            // copy all current contents
            /// TODO: TsQueue can be copied?
            PLEEPLOG_DEBUG("copy timestream data");
            //destination->m_timestreams = source->m_timestreams;

            PLEEPLOG_DEBUG("set link pointers");
            // link source parallel
            source->m_linkedTimestreamMap = destination;
            // link destination parallel (for unlinking)
            destination->m_linkedTimestreamMap = source;
        }

        // deadlock avoiding assignment for both timestreams
        static void unlink_timestreams(std::shared_ptr<EntityTimestreamMap> toUnlink)
        {
            if (toUnlink->m_linkedTimestreamMap == nullptr) return;

            {
                const std::lock_guard<std::mutex> lk(toUnlink->m_linkedTimestreamMap->m_mapMux);
                toUnlink->m_linkedTimestreamMap->m_linkedTimestreamMap = nullptr;
            }

            {
                const std::lock_guard<std::mutex> lk(toUnlink->m_mapMux);
                toUnlink->m_linkedTimestreamMap = nullptr;
            }
        }

        // Wrap message Queue methods to maintain timestamp ordering.

        // If no stream exists emplace it.
        // No strict ordering of messages enforced, msg coherency will be use for pop availability
        // coherency is circular so there is no catch-all default value
        void push_to_timestream(Entity entity, Message<EventId> msg)
        {
            const std::lock_guard<std::mutex> lk(m_mapMux);

            // TODO: detect when timstreams has gone way beyond expected capacity and stop pushing
            //PLEEPLOG_DEBUG("Pushing event: " + std::to_string(msg.header.id) + " for entity: " + std::to_string(entity) + " at coherency: " + std::to_string(msg.header.coherency));

            // operator[] emplaces with default constructor for TsQueue
            m_timestreams[entity].push_back(msg);

            // if a parallel timestream is registered then push to it
            if (m_linkedTimestreamMap)
            {
                // dont recurse, do it directly
                m_linkedTimestreamMap->m_timestreams[entity].push_back(msg);
            }
        }

        // check if timestream exists for an entity, if it is non-empty,
        // AND if the front value has a coherency <= currentCoherency
        bool entity_has_data(Entity entity, uint16_t currentCoherency)
        {
            const std::lock_guard<std::mutex> lk(m_mapMux);

            return is_data_available(entity, currentCoherency);
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
        bool pop_from_timestream(Entity entity, uint16_t currentCoherency, Message<EventId>& dest)
        {
            const std::lock_guard<std::mutex> lk(m_mapMux);

            // check if data is available internally to avoid lock juggling
            if (!is_data_available(entity, currentCoherency)) return false;
            //PLEEPLOG_DEBUG("Popping for entity: " + std::to_string(entity) + " on coherency: " + std::to_string(currentCoherency) + ". There are " + std::to_string(m_timestreams.at(entity).count()) + " messages.");

            return m_timestreams.at(entity).pop_front(dest);

            // TODO: either check for empty timestream or check for when we pop a ENTITY_REMOVED
            //   message and remove index (entity) from timestream map
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

    private:
        // check if timestream exists for an entity, if it is non-empty,
        // and if the front value has a coherency <= currentCoherency
        // without holding the lock
        bool is_data_available(Entity entity, uint16_t currentCoherency)
        {
            // no lock, only for internal use
            auto timestreams_it = m_timestreams.find(entity);
            return timestreams_it != m_timestreams.end()
                && !timestreams_it->second.empty()
                && coherency_greater_or_equal(currentCoherency, timestreams_it->second.front().header.coherency);
        }

        std::unordered_map<Entity, Timestream> m_timestreams;

        std::mutex m_mapMux;

        // parallel timestream map to duplicate pushed messages into
        std::shared_ptr<EntityTimestreamMap> m_linkedTimestreamMap;
    };
}

#endif // ENTITY_TIMESTREAM_MAP_H