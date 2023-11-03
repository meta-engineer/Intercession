#include "timeline_api.h"

namespace pleep
{
    TimelineApi::TimelineApi(TimelineConfig cfg, TimesliceId id, 
            std::shared_ptr<Multiplex> sharedMultiplex, 
            std::shared_ptr<EntityTimestreamMap> pastTimestreams,
            std::shared_ptr<EntityTimestreamMap> futureTimestreams)
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

        // indexes into sharedMultiplex
        m_timesliceId = id;

        // store whole cfg, or just copy individual members?
        m_delayToNextTimeslice = cfg.timesliceDelay;
        m_simulationHz = cfg.simulationHz;

        // offset port in series by unique timeslice id
        m_port = cfg.presentPort + m_timesliceId;
    }

    // get the unique timesliceId registered for this TimelineApi instance
    TimesliceId TimelineApi::get_timeslice_id()
    {
        return m_timesliceId;
    }

    // get total number of timeslices in the local network (ids SHOULD start from 0)
    size_t TimelineApi::get_num_timeslices()
    {
        return m_multiplex->size();
    }

    uint16_t TimelineApi::get_port()
    {
        return m_port;
    }

    uint16_t TimelineApi::get_timeslice_delay()
    {
        return m_delayToNextTimeslice;
    }

    uint16_t TimelineApi::get_simulation_hz()
    {
        return m_simulationHz;
    }

    bool TimelineApi::send_message(TimesliceId id, EventMessage& data)
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

    bool TimelineApi::is_message_available()
    {
        return !(m_multiplex->at(m_timesliceId).empty());
    }
    bool TimelineApi::pop_message(EventMessage& dest)
    {
        // MUST ONLY POP FROM m_timesliceId
        return m_multiplex->at(m_timesliceId).pop_front(dest);
    }

    bool TimelineApi::has_future()
    {
        return m_futureTimestreams != nullptr;
    }
    bool TimelineApi::has_past()
    {
        return m_pastTimestreams != nullptr;
    }

    void TimelineApi::push_past_timestream(Entity entity, Message<EventId>& data)
    {
        if (!m_pastTimestreams)
        {
            PLEEPLOG_WARN("This timeslice has no past timestream to push to");
            return;
        }
        m_pastTimestreams->push_to_timestream(entity, data);

        // Also push data to an ongoing past parallel timstream

    }

    void TimelineApi::clear_past_timestream(Entity entity)
    {
        if (!m_pastTimestreams)
        {
            PLEEPLOG_WARN("This timeslice has no past timestream to clear");
            return;
        }
        m_pastTimestreams->clear(entity);
    }

    std::vector<Entity> TimelineApi::get_entities_with_future_streams()
    {
        if (!m_futureTimestreams)
        {
            PLEEPLOG_WARN("This timeslice has no future timestream at all!");
            return std::vector<Entity>{};
        }

        return m_futureTimestreams->get_entities_with_streams();
    }

    bool TimelineApi::pop_future_timestream(Entity entity, uint16_t coherency, EventMessage& dest)
    {
        if (!m_futureTimestreams)
        {
            PLEEPLOG_WARN("This timeslice has no future timestream to pop from");
        }
        return m_futureTimestreams->pop_from_timestream(entity, coherency, dest);
    }



    void TimelineApi::future_parallel_init_and_start(std::shared_ptr<Cosmos> src)
    {
        // if thread is already running then return?
        if (!future_parallel_is_closed()) return;

        PLEEPLOG_INFO("Starting parallel simulation");

        // - deep copy src into parallel
        m_futureParallelContext->copy_cosmos(src);
        // - deep copy upstream components from timestream
        m_futureParallelContext->copy_timestreams(m_futureTimestreams);
        // - set coherency target for parallel: current coherency + 1 (next frame) + delay to next timeslice
        m_futureParallelContext->set_coherency_target(src->get_coherency() + 1 + m_delayToNextTimeslice);
        // - start parallel cosmos running on new thread
        m_futureParallelContext->start();
    }

    bool TimelineApi::future_parallel_is_closed()
    {
        // not only should thread be joined, but also cosmos should be closed
        return !m_futureParallelContext->cosmos_exists();
    }

    void TimelineApi::past_parallel_close()
    {
        // wait for thread to join
        m_pastParallelContext->join();
        
        // then close cosmos & timestreams
        m_pastParallelContext->close();
    }

    bool TimelineApi::past_parallel_is_closed()
    {
        // not only should thread be joined, but also cosmos should be closed
        return !m_pastParallelContext->cosmos_exists();
    }

    void TimelineApi::past_parallel_set_target_coherency(uint16_t newTarget)
    {
        m_pastParallelContext->set_coherency_target(newTarget);
        // also restart thread if it had stopped
        m_pastParallelContext->start();
    }

    uint16_t TimelineApi::past_parallel_get_current_coherency()
    {
        // should return 0 if simulation is still running?
        if (m_pastParallelContext->is_running()) return 0;
        return m_pastParallelContext->get_current_coherency();
    }

    const std::vector<Entity> TimelineApi::past_parallel_get_forked_entities()
    {
        // must be maintained ordered by occurance, consistent between calls
        return m_pastParallelContext->get_forked_entities();
    }

    EventMessage TimelineApi::past_parallel_extract_entity(Entity e)
    {
        // serialize into message to transfar to local cosmos
        return m_pastParallelContext->extract_entity(e);
    }
}