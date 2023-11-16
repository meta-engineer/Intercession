#ifndef SUPERPOSITION_RELAY_H
#define SUPERPOSITION_RELAY_H

//#include "intercession_pch.h"

#include <vector>
#include <unordered_set>
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
                m_resolutionCandidates.insert(data.entity);
            }
            else
            {
                m_nonCandidates.insert(data.entity);
            }

            // check to trigger resolution
            if (data.spacetime.timestreamStateCoherency + FORKED_THRESHOLD_MAX <= m_lastCoherency)
            {
                //PLEEPLOG_DEBUG("Entity " + std::to_string(data.entity) + " is due for resolution at: " + std::to_string(m_lastCoherency));
                m_resolutionNeeded = true;
            }
        }

        void engage(double deltaTime, std::weak_ptr<Cosmos> localCosmos, TimelineApi* localTimelineApi)
        {
            UNREFERENCED_PARAMETER(deltaTime);

            std::shared_ptr<Cosmos> cosmos = localCosmos.lock();

            // store coherency for next frame
            m_lastCoherency = cosmos->get_coherency();

            // A: forked entities need to be sent into FUTURE parallel cosmos
            //      if future parallel context is not ready (still running or still has update data), skip
            //      else track candidates and when time threshold passes setup & start future cosmos
            if (localTimelineApi->has_future() &&
                localTimelineApi->future_parallel_is_closed() &&
                m_resolutionNeeded)
            {
                PLEEPLOG_DEBUG("Starting parallel cosmos for " + std::to_string(m_resolutionCandidates.size()) + " entities");
                localTimelineApi->future_parallel_init_and_start(cosmos, m_resolutionCandidates, m_nonCandidates);
                // future_parallel_joined should now return false
                assert(localTimelineApi->future_parallel_is_closed() == false);

                // set our local entities back to merged (just delete their spacetime component?)
                // and clear their future timestream
                for (Entity e : m_resolutionCandidates)
                {
                    // dangerous to remove component during update?
                    cosmos->get_component<SpacetimeComponent>(e).timestreamState = TimestreamState::merged;
                    cosmos->get_component<SpacetimeComponent>(e).timestreamStateCoherency = cosmos->get_coherency();

                    ///// TODO: leave timestream until resolution, but only playback upstream?
                    localTimelineApi->clear_future_timestream(e);
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
                // this should never happen, but just in case...
                if (m_forkedCursor > forkedEntities.size()) m_forkedCursor = 0;

                // if list has expanded since last fetch then signal these as in superposition
                if (forkedEntities.size() > m_forkedCursor)
                {
                    // loop through only from last cursor position
                    auto forkedEntitiesIt = forkedEntities.begin() + m_forkedCursor;
                    for (; m_forkedCursor < forkedEntities.size(); m_forkedCursor++)
                    {
                        EventMessage superpositionMessage(events::cosmos::TIMESTREAM_INTERCEPTION);
                        events::cosmos::TIMESTREAM_INTERCEPTION_params superpositionInfo{
                            NULL_ENTITY,
                            forkedEntities[m_forkedCursor]
                        };
                        superpositionMessage << superpositionInfo;
                        // send interception event to OURSELVES on timeline api
                        // entities are already from the past, so chainlink is pre-decremented
                        localTimelineApi->send_message(localTimelineApi->get_timeslice_id(), superpositionMessage);
                    }
                }

                // check if parallel has reached coherency target (also check if simulation has stopped?)
                // if yes, then extract all forked entities
                // if no, then set target coherency to next frame (and restart thread if needed)
                if (localTimelineApi->past_parallel_get_current_coherency() == cosmos->get_coherency())
                {
                    PLEEPLOG_DEBUG("Past parallel synchronized at coherency: " + std::to_string(cosmos->get_coherency()));
                    PLEEPLOG_DEBUG("Try to extract " + std::to_string(forkedEntities.size()) + " entities for superposition resolution");
                    for (Entity e : forkedEntities)
                    {
                        // extract Entity update for each forked entity
                        EventMessage entityUpdate;
                        if (!localTimelineApi->past_parallel_extract_entity(e, entityUpdate))
                        {
                            PLEEPLOG_DEBUG("Extract failed!?!?!?!?!?!?!?");
                            continue;
                        }
                        assert(entityUpdate.header.id == events::cosmos::ENTITY_UPDATE);

                        events::cosmos::ENTITY_UPDATE_params updateInfo;
                        entityUpdate >> updateInfo;
                        // decrement chainlink to update entity's future self
                        if (!decrement_causal_chain_link(updateInfo.entity))
                        {
                            PLEEPLOG_ERROR("Forked entity could not have chainlink decremented. How did it get forked to begin with if it doesn't have a future?");
                            assert(false);
                        }

                        // send it to our cosmos
                        cosmos->deserialize_entity_components(updateInfo.entity, updateInfo.sign, updateInfo.subset, entityUpdate);
                        // set timestream state to merged (or just remove)
                        if (cosmos->has_component<SpacetimeComponent>(updateInfo.entity))
                        {
                            cosmos->get_component<SpacetimeComponent>(updateInfo.entity).timestreamState = TimestreamState::merged;
                            cosmos->get_component<SpacetimeComponent>(updateInfo.entity).timestreamStateCoherency = cosmos->get_coherency();
                            PLEEPLOG_DEBUG("Resolved entity " + std::to_string(updateInfo.entity) + " superposition");
                        }
                        else
                        {
                            PLEEPLOG_WARN("Could not find SpacetimeComponent for entity " + std::to_string(updateInfo.entity) + " despite it being in the forked set?");
                        }
                    }
                    // "close" simulation to signal to past that it is overwritable
                    localTimelineApi->past_parallel_close();
                    // forkedEntities will be cleared
                    m_forkedCursor = 0;
                }
                else
                {
                    // Not at coherency target so increase coherency target to next frame and let it continue
                    //PLEEPLOG_DEBUG("Parallel coherency " + std::to_string(localTimelineApi->past_parallel_get_current_coherency()) + " != local coherency " + std::to_string(cosmos->get_coherency()));
                    localTimelineApi->past_parallel_set_target_coherency(cosmos->get_coherency() + 1);
                    // new upstream components should be pushed ongoing into past parallel timestream via TimelineApi::push_past_timstream
                }
            }
        }

        void clear()
        {
            m_resolutionCandidates.clear();
            m_nonCandidates.clear();
            m_resolutionNeeded = false;
            // keep m_lastCoherency for next frame's submittions
            // only clear m_forkedCursor when past parallel is closed
        }

    protected:
        // store forked entities which have passed min threshold each frame
        std::unordered_set<Entity> m_resolutionCandidates;
        // store forked entities which are not yet ready to resolve
        std::unordered_set<Entity> m_nonCandidates;
        // note if a submitted entity candidate is ready for resolution
        bool m_resolutionNeeded = false;

        // store coherency at previous frame to check submittions for next frame
        uint16_t m_lastCoherency = 0;
        
        // track index in current past parallel's forked list
        size_t m_forkedCursor = 0;
    };
}

#endif // SUPERPOSITION_RELAY_H