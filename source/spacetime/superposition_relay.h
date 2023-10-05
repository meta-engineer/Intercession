#ifndef SUPERPOSITION_RELAY_H
#define SUPERPOSITION_RELAY_H

//#include "intercession_pch.h"

#include <vector>
#include <assert.h>

#include "core/i_cosmos_context.h"
#include "spacetime/spacetime_packet.h"
#include "spacetime/parallel_cosmos_context.h"

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

        void engage(double deltaTime, uint16_t coherency)
        {
            UNREFERENCED_PARAMETER(deltaTime);
            // store coherency for next frame
            m_lastCoherency = coherency;

            // Check if parallelContext IS done (reached coherency target) AND has data available
            //   check which entities were forked, set them as merged to clear superposition
            //   and send as entity resolutions to future.
            //   (dynamo must pass us access to timelineApi to do this?)
            if (!m_parallelContext.is_running() && m_parallelContext.has_update_available())
            {

            }

            
            // A: if parallel IS NOT done
            //   then update simulation:
            if (m_parallelContext.is_running())
            {
                // A.1: increase coherency target to keep synced with future timeslice
                //   current + 1 (next frame) + timeslice_offset + timestream latency (1?)

                // A.2: push new upstream components from future timestream (how to fetch these?)

                // A.3: for any newly forked entities (including the initial set) send an interception event to future to refresh their superposition state

            }
            // B: if parallel IS done AND it has no data to distribute AND new resolution is needed
            //   then start new simulation:
            else if (!m_parallelContext.is_running() && !m_parallelContext.has_update_available() && m_resolutionNeeded)
            {
                // B.1: copy current cosmos into parallel

                // B.2: set coherency target for parallel: 
                //   current + 1 (next frame) + timeslice_offset + timestream latency (1?)

                // B.3: start parallel cosmos running on new thread

                // B.4: set our local entities back to merged 
                //   (delete their spacetime component? would need access to cosmos)
                //cosmos->remove_component<SpacetimeComponent>(entity);
            }
            // C: Otherwise, no entities are ready for resolution so just continue until next frame
        }

        void clear()
        {
            m_resolutionCandidates.clear();
            m_resolutionNeeded = false;
        }

    protected:
        // parallel sub-simulation to use if a superposition entity is ready
        ParallelCosmosContext m_parallelContext;

        // store forked entities which have passed min threshold each frame
        std::vector<Entity> m_resolutionCandidates;

        // note if a submitted entity candidate is ready for resolution
        bool m_resolutionNeeded = false;

        // store coherency at previous frame to check submittions for next frame
        uint16_t m_lastCoherency = static_cast<uint16_t>(-1);
    };
}

#endif // SUPERPOSITION_RELAY_H