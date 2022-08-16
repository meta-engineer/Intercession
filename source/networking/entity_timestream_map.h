#ifndef ENTITY_TIMESTREAM_MAP_H
#define ENTITY_TIMESTREAM_MAP_H

//#include "intercession_pch.h"
//#include <deque>
#include <unordered_map>

#include "networking/ts_queue.h"
#include "networking/net_message.h"
#include "ecs/ecs_types.h"

namespace pleep
{
    // Track entities and components through time
    // Every Network tick will update clients as well as update
    // a local set of past timestreams for each entity.
    // Once timestream messages exceed ~half the timeslice delay they are forwarded to
    // the child server who buffers them into their future timestream to be processed

    // each timestream should be associated to one entity
    // they should contain a set of timestamps with entity update messages 
    // ORDERED by timestamp. It needs to be back pushable and front poppable.
    // Also, messages beyond the front need to be interrogatable (for latency compensation)
    // https://www.reddit.com/r/Overwatch/comments/3u5kfg/everything_you_need_to_know_about_tick_rate/
    // They do NOT need to be randomly indexable (exact timestamp will not be knowable)
    // But may need to be searchable. EX: the first timestamp after X time.
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

        using Timestream = TsQueue<TimestampedMessage<T_Msg>>;

        // Wrap message Queue methods to maintain timestamp ordering.

    private:
        // do we need use of deque iterator? or do we need threadsafe?
        // we could provide specific extra threadsafe operations like searches.
        //TsQueue<TimestampedMessage<T_Msg>> m_messageDeque;

        // maybe this should be both levels combined
        //std::unordered_map<Entity, TsQueue<TimestampedMessage<T_Msg>>> m_timestreams;
    };
}

#endif // ENTITY_TIMESTREAM_MAP_H