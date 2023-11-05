#ifndef SUPERPOSITION_RELAY_H
#define SUPERPOSITION_RELAY_H

//#include "intercession_pch.h"

#include <vector>
#include <assert.h>

#include "core/i_cosmos_context.h"
#include "spacetime/spacetime_packet.h"
#include "networking/timeline_api.h"

namespace pleep
{
    // number of frames a forked state has lasted where it becomes a candidate for resolution
    constexpr uint16_t FORKED_THRESHOLD_MIN = static_cast<uint16_t>(0.5 * FRAMERATE);
    // number of frames a forked state has lasted which triggers a resolution
    constexpr uint16_t FORKED_THRESHOLD_MAX = static_cast<uint16_t>(2.0 * FRAMERATE);
    // if max is less than min, simulations will be triggered with no candidates
    static_assert(FORKED_THRESHOLD_MAX >= FORKED_THRESHOLD_MIN);

    class SuperpositionRelay
    {
    public:
        SuperpositionRelay() = default;
        ~SuperpositionRelay() = default;

        void submit(SpacetimePacket data)
        {
            // check if forked
            if (data.spacetime.timestreamState != TimestreamState::forked) return;
            // thus we can assume its future is in a superposition

            // check coherency timing to add to candidates
            if (data.spacetime.timestreamStateCoherency + FORKED_THRESHOLD_MIN <= m_lastCoherency)
            {
                m_resolutionCandidates.push_back(data.entity);
            }

            // check to trigger resolution
            if (data.spacetime.timestreamStateCoherency + FORKED_THRESHOLD_MAX <= m_lastCoherency)
            {
                m_resolutionNeeded = true;
            }
        }

        void engage(double deltaTime, std::weak_ptr<Cosmos> localCosmos, TimelineApi* localTimelineApi)
        {
            UNREFERENCED_PARAMETER(deltaTime);

            std::shared_ptr<Cosmos> cosmos = localCosmos.lock();

            // store coherency for next frame
            m_lastCoherency = cosmos->get_coherency();

            if (m_resolutionNeeded) PLEEPLOG_DEBUG("superposition resolution needed this frame");

            // A: forked entities need to be sent into FUTURE parallel cosmos
            //      if future parallel context is not ready (still running or still has update data), skip
            //      else track candidates and when time threshold passes setup & start future cosmos
            if (localTimelineApi->has_future() &&
                localTimelineApi->future_parallel_is_closed() &&
                m_resolutionNeeded)
            {
                localTimelineApi->future_parallel_init_and_start(cosmos);
                // future_parallel_joined should now return false
                assert(localTimelineApi->future_parallel_is_closed() == false);

                // set our local entities back to merged (just delete their spacetime component?)
                for (Entity e : m_resolutionCandidates)
                {
                    cosmos->remove_component<SpacetimeComponent>(e);
                }
            }

            // B: superposition entities need to be updated from PAST parallel cosmos
            //      if parallel is running (coherency target is not reached), update coherency target to next frame
            //      any entities which have entered forked state inside parallel should be set in superposition locally
            //      if parallel reached coherency target, pop entity updates and pass into local cosmos
            if (localTimelineApi->has_past() &&
                !localTimelineApi->past_parallel_is_closed())
            {
                // fetch forked entity list
                const std::vector<Entity> forkedEntities = localTimelineApi->past_parallel_get_forked_entities();
                // this should never happen, but just incase...
                if (m_forkedCursor > forkedEntities.size()) m_forkedCursor = 0;

                // if list has expanded since last fetch then signal these as in superposition
                if (forkedEntities.size() > m_forkedCursor)
                {
                    // loop through only from last cursor position
                    for (; m_forkedCursor < forkedEntities.size(); m_forkedCursor++)
                    {
                        EventMessage superpositionMessage(events::cosmos::TIMESTREAM_INTERCEPTION);
                        events::cosmos::TIMESTREAM_INTERCEPTION_params superpositionInfo{
                            NULL_ENTITY,
                            forkedEntities[m_forkedCursor]
                        };
                        superpositionMessage << superpositionInfo;
                        // send interception event OURSELVES on timeline api
                        localTimelineApi->send_message(localTimelineApi->get_timeslice_id(), superpositionMessage);
                    }
                }


                // check if parallel has reached coherency target (also check if simulation has stopped?)
                // if yes, then extract all forked entities
                // if no, then set target coherency to next frame (and restart thread if needed)
                if (localTimelineApi->past_parallel_get_current_coherency() == cosmos->get_coherency())
                {
                    PLEEPLOG_DEBUG("Past parallel synchronized at coherency: " + std::to_string(cosmos->get_coherency()));
                    for (Entity e : forkedEntities)
                    {
                        // extract Entity update for each forked entity
                        EventMessage entityUpdate;
                        if (!localTimelineApi->past_parallel_extract_entity(e, entityUpdate))
                        {
                            continue;
                        }
                        assert(entityUpdate.header.id == events::cosmos::ENTITY_UPDATE);
                        // send it to our cosmos
                        events::cosmos::ENTITY_UPDATE_params updateInfo;
                        entityUpdate >> updateInfo;
                        cosmos->deserialize_entity_components(updateInfo.entity, updateInfo.sign, updateInfo.subset, entityUpdate);
                    }
                    PLEEPLOG_DEBUG("Extracted " + std::to_string(forkedEntities.size()) + " entities for superposition resolution");
                    // "close" simulation to signal to past that it is overwritable
                    localTimelineApi->past_parallel_close();
                    // forkedEntities will be cleared
                    m_forkedCursor = 0;
                }
                else
                {
                    // Not at coherency target so increase coherency target to next frame and let it continue
                    localTimelineApi->past_parallel_set_target_coherency(cosmos->get_coherency() + 1);

                    // new upstream components should be pushed ongoing into past parallel timestream via TimelineApi::push_past_timstream
                }
            }
        }

        void clear()
        {
            m_resolutionCandidates.clear();
            m_resolutionNeeded = false;
            // keep m_lastCoherency for next frame's submittions
            // only clear m_forkedCursor when past parallel is closed
        }

    protected:
        // store forked entities which have passed min threshold each frame
        std::vector<Entity> m_resolutionCandidates;
        // note if a submitted entity candidate is ready for resolution
        bool m_resolutionNeeded = false;

        // store coherency at previous frame to check submittions for next frame
        uint16_t m_lastCoherency = 0;
        
        // track index in current past parallel's forked list
        size_t m_forkedCursor = 0;
    };
}

#endif // SUPERPOSITION_RELAY_H