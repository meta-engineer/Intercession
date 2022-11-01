#ifndef TIMELINE_API_H
#define TIMELINE_API_H

//#include "intercession_pch.h"
#include <utility>
#include <memory>
#include <unordered_map>
#include <cassert>
#include <string>

#include "logging/pleep_log.h"
#include "networking/ts_queue.h"
#include "networking/timeline_config.h"
#include "networking/timeline_types.h"
#include "networking/entity_timestream_map.h"
#include "events/event_types.h"

namespace pleep
{
    // Provide all "calibratable" parameters for a single thread (and defaults)
    // Should also have a validator function to check values
    // Provide communication channels/addresses to access other timeslices
    // specifies EventMessage as transferable datatype
    class TimelineApi
    {
    public:
        // Should queue type contain the source id? Otherwise it has to be built into message manually
        // Hard-code message type to use EventId (to avoid template cascading)
        using Multiplex = std::unordered_map<TimesliceId, TsQueue<Message<EventId>>>;

        // Accept top level timeline config, and my individual timesliceId
        // accept Message Conduits shared for each individual api?
        TimelineApi(TimelineConfig cfg, TimesliceId id, 
                    std::shared_ptr<Multiplex> sharedMultiplex, 
                    std::shared_ptr<EntityTimestreamMap> pastTimestreams = nullptr,
                    std::shared_ptr<EntityTimestreamMap> futureTimestreams = nullptr)
            : m_multiplex(sharedMultiplex)
            , m_pastTimestreams(pastTimestreams)
            , m_futureTimestreams(futureTimestreams)
        {
            if (m_multiplex->find(id) == m_multiplex->end())
            {
                std::string errMsg = ("TimelineApi was constructed with an id (" + std::to_string(id) + ") which does not exist in the provided Multiplex");
                PLEEPLOG_ERROR(errMsg);
                throw std::range_error(errMsg);
            }

            m_timesliceId = id;

            // store whole cfg, or just copy individual members?
            m_delayToNextTimeslice = cfg.timesliceDelay;
            m_simulationHz = cfg.simulationHz;
            m_networkHz = cfg.networkHz;

            // offset port in series by unique timeslice id
            m_port = cfg.originPort + m_timesliceId;
        }

        // get the unique timesliceId registered for this TimelineApi instance
        TimesliceId get_timeslice_id()
        {
            return m_timesliceId;
        }

        // get total number of timeslices in the local network (ids SHOULD start from 0)
        size_t get_num_timeslices()
        {
            return m_multiplex->size();
        }

        uint16_t get_port()
        {
            return m_port;
        }

        // ***** Accessors for multiplex *****

        // Restrict access to outgoing queues to only be sendable
        bool send_message(TimesliceId id, EventMessage& data)
        {
            // sending to my own id is ok, because we can re-use the same logic on receiving
            if (id == m_timesliceId)
            {
                PLEEPLOG_WARN("Trying to send data to my own id (" + std::to_string(id) + "). Is this intended?");
            }

            auto outgoingQueueIt = m_multiplex->find(id);
            if (outgoingQueueIt == m_multiplex->end())
            {
                PLEEPLOG_WARN("Trying to send data to non-existent id (" + std::to_string(id) + "). Only " + std::to_string(get_num_timeslices()) + " total ids exist.");
                return false;
            }
            
            outgoingQueueIt->second.push_back(data);
            return true;
        }

        // check to prevent popping empty deque
        bool is_message_available()
        {
            return !(m_multiplex->at(m_timesliceId).empty());
        }
        // Restrict access to incoming queue to only be receivable
        // Has same exception behaviour of deque on popping empty deque
        EventMessage pop_message()
        {
            // MUST ONLY POP FROM m_timesliceId
            return m_multiplex->at(m_timesliceId).pop_front().first;
        }

        // ***** Accessors for Timestreams *****

        // detect if I have parent/child timeslices
        // ids may not necessarily be in order or strictly less than the config size
        bool has_future()
        {
            return m_futureTimestreams != nullptr;
        }
        bool has_past()
        {
            return m_pastTimestreams != nullptr;
        }

        // because event type is ambiguous, caller must specify entity regardless
        void push_past_timestream(Entity entity, TimestampedMessage<EventId>& data)
        {
            if (!m_pastTimestreams)
            {
                PLEEPLOG_WARN("This timeslice has no past timestream to push to");
                return;
            }
            m_pastTimestreams->push_to_timestream(entity, data);
        }

        // Restrict access to future timestreams to only be receivable
        // Has same exception behaviour of deque on popping empty deque
        EventMessage pop_future_timestream(Entity entity)
        {
            if (!m_futureTimestreams)
            {
                PLEEPLOG_WARN("This timeslice has no future timestream to pop from");
            }
            return m_futureTimestreams->pop_from_timestream(entity).msg;
        }

    private:
        TimesliceId m_timesliceId;   // Store for TemporalEntity calculation
        int m_delayToNextTimeslice;  // SECONDS
        
        double m_simulationHz;       // updates per second
        double m_networkHz;

        std::shared_ptr<Multiplex> m_multiplex;

        std::shared_ptr<EntityTimestreamMap> m_futureTimestreams;
        std::shared_ptr<EntityTimestreamMap> m_pastTimestreams;

        uint16_t m_port;
    };

    // return a multiplex map with empty queues for ids 0 to (numUsers - 1)
    // this should be passed to each TimelineApi, along with each unique id
    inline std::shared_ptr<TimelineApi::Multiplex> generate_timeline_multiplex(TimesliceId numUsers)
    {
        std::shared_ptr<TimelineApi::Multiplex> multiplex = std::make_shared<TimelineApi::Multiplex>();
        for (TimesliceId i = 0; i < numUsers; i++)
        {
            // emplace with default constructor for TsQueue
            multiplex->operator[](i);
        }
        
        return multiplex;
    }
}

#endif // TIMELINE_API_H