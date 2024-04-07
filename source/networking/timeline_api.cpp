#include "timeline_api.h"

#include "spacetime/parallel_cosmos_context.h"

namespace pleep
{
    TimelineApi::TimelineApi(TimelineConfig cfg, TimesliceId id, 
            std::shared_ptr<Multiplex> sharedMultiplex, 
            std::shared_ptr<EntityTimestreamMap> pastTimestreams,
            std::shared_ptr<EntityTimestreamMap> futureTimestreams,
            std::shared_ptr<ParallelCosmosContext> parallelContext)
        : m_multiplex(sharedMultiplex)
        , m_pastTimestreams(pastTimestreams)
        , m_futureTimestreams(futureTimestreams)
        , m_sharedParallel(parallelContext)
    {
        if (id != NULL_TIMESLICEID && m_multiplex->find(id) == m_multiplex->end())
        {
            std::string errMsg = ("TimelineApi was constructed with an id (" + std::to_string(id) + ") which does not exist in the provided Multiplex");
            PLEEPLOG_ERROR(errMsg);
            throw std::range_error(errMsg);
        }

        // indexes into sharedMultiplex
        m_timesliceId = id;

        // store whole cfg, or just copy individual members?
        m_delayToNextTimeslice = cfg.timesliceDelay;

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

    bool TimelineApi::send_message(TimesliceId id, const EventMessage& data)
    {
        // sending to my own id is ok, because we can re-use the same logic on receiving
        /*
        if (id == m_timesliceId)
        {
            PLEEPLOG_WARN("Trying to send data to my own id (" + std::to_string(id) + "). Is this intended?");
        }
        */

        auto outgoingQueueIt = m_multiplex->find(id);
        if (outgoingQueueIt == m_multiplex->end())
        {
            PLEEPLOG_WARN("Trying to send data to non-existent id (" + std::to_string(id) + "). Only " + std::to_string(get_num_timeslices()) + " total ids exist.");
            return false;
        }
        
        outgoingQueueIt->second.push_back(data);
        return true;
    }

    bool TimelineApi::broadcast_message(const EventMessage& data)
    {
        for (auto& msgQ : *m_multiplex)
        {
            if (msgQ.first == m_timesliceId) continue;

            msgQ.second.push_back(data);
        }
        // is there any "false" condition?
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

    void TimelineApi::push_past_timestream(Entity entity, const EventMessage& data)
    {
        if (!m_pastTimestreams)
        {
            PLEEPLOG_WARN("This timeslice has no past timestream to push to");
            return;
        }
        m_pastTimestreams->push_to_timestream(entity, data);
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
    
    void TimelineApi::link_timestreams(std::shared_ptr<EntityTimestreamMap> sourceTimestreams)
    {
        // clear breakpoints of old streams which we linked to
        if (m_futureTimestreams) m_futureTimestreams->remove_breakpoints();
        if (m_pastTimestreams) m_pastTimestreams->remove_breakpoints();

        // set breakpoint to start
        if (sourceTimestreams) sourceTimestreams->set_breakpoints();

        // do both so that has_past and has_future work as expected
        m_futureTimestreams = sourceTimestreams;
        m_pastTimestreams = sourceTimestreams;
    }
    
    void TimelineApi::push_timestream_at_breakpoint(Entity entity, const EventMessage& data)
    {
        if (!m_pastTimestreams)
        {
            PLEEPLOG_WARN("This timeslice has no past timestream to push to");
            return;
        }
        m_pastTimestreams->push_to_timestream_at_breakpoint(entity, data);
    }
    bool TimelineApi::pop_timestream_at_breakpoint(Entity entity, uint16_t coherency, EventMessage& dest)
    {
        // use the "future" timestream
        if (!m_futureTimestreams)
        {
            PLEEPLOG_WARN("This timeslice has no future timestream to pop from");
            return false;
        }
        return m_futureTimestreams->pop_from_timestream_at_breakpoint(entity, coherency, dest);
    }


    void TimelineApi::parallel_notify_divergence()
    {
        if (m_sharedParallel == nullptr) return;
        m_sharedParallel->request_resolution(m_timesliceId);
    }

    void TimelineApi::parallel_load_and_link(const std::shared_ptr<Cosmos> sourceCosmos)
    {
        if (m_sharedParallel == nullptr) return;
        // parallel should stop us if it is already running for some reason

        PLEEPLOG_TRACE("Initing parallel cosmos");

        // deep copy cosmos and timestream into parallel
        if (!m_sharedParallel->load_and_link(sourceCosmos, m_futureTimestreams))
        {
            PLEEPLOG_WARN("parallel loading failed... somehow? Ignoring...");
        }
    }

    void TimelineApi::parallel_retarget(uint16_t newTarget)
    {
        if (m_sharedParallel == nullptr) return;
        //PLEEPLOG_DEBUG("Updating parallel coherency target to " + std::to_string(newTarget));
        m_sharedParallel->set_coherency_target(newTarget);
    }

    void TimelineApi::parallel_start()
    {
        if (m_sharedParallel == nullptr) return;

        // try restart thread only if it had stopped
        if (!m_sharedParallel->is_running())
        {
            PLEEPLOG_DEBUG("Restarting parallel...");
            // start() is idempotent if already running
            m_sharedParallel->start();
        }
    }
    
    TimesliceId TimelineApi::parallel_get_timeslice()
    {
        return m_sharedParallel->get_current_timeslice();
    }

    bool TimelineApi::parallel_extract(std::shared_ptr<Cosmos> dstCosmos)
    {
        if (m_sharedParallel == nullptr) return false;
        return m_sharedParallel->extract_entity_updates(dstCosmos);
    }
}