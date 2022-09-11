#ifndef ENTITY_TIMESTREAM_MAP_H
#define ENTITY_TIMESTREAM_MAP_H

//#include "intercession_pch.h"
//#include <deque>
#include <unordered_map>

#include "networking/ts_queue.h"
#include "networking/net_message.h"
#include "networking/timeline_types.h"

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

    template <typename T_Msg>
    class EntityTimestreamMap
    {
    public:
        EntityTimestreamMap();
        ~EntityTimestreamMap();

        // Timestream could be it's own class to manage Message ordering
        // do we need use of deque iterators? or do we need threadsafe?
        // we could provide specific extra threadsafe operations like searches to maintain threadsafety
        using Timestream = TsQueue<TimestampedMessage<T_Msg>>;

        // Wrap message Queue methods to maintain timestamp ordering.

        // TODO: Can we know which queues have messages available without iterating through the whole map?
        // We need a way to get every message for any entity which is ready (dependant on system clock?
        // or dependant on semi-regular intercessionAppInfo updates with system clock dead-reckoning)
        // and then pass each event to eventBroker (maybe use network dynamo's?)
        // is there any better method than iterating throught the whole map and checkign timestamps?

        // If no stream exists emplace it.
        void push_to_timestream(Entity entity, TimestampedMessage<T_Msg> msg)
        {
            if (m_timestreams.find(entity) != m_timestreams.end())
            {
                m_timestreams.emplace(entity);
            }

            m_timestreams.at(entity).push_back(msg);
        }

        // check if timestream exists for an entity and if it is non-empty
        bool is_data_available(Entity entity)
        {
            return m_timestreams.find(entity) != m_timestreams.end()
                && !m_timestreams.at(entity).empty();
        }

    private:
        std::unordered_map<Entity, Timestream> m_timestreams;
    };
}

#endif // ENTITY_TIMESTREAM_MAP_H