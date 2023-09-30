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

            // check if parallelContext is done and send entity resolutions to future
            // dynamo must pass us access to timelineApi?

            
            // if not done update with:
            //   increased coherency target to keep synced with future in realtime
            //   upstream components from future timestream (how to fetch these?)


            // start new simulation if needed
            if (!m_resolutionNeeded) return;

            // copy current cosmos into parallel

            // store which entities are being resolved in parallel (for access above)

            // set coherency target for parallel? current + timeslice_offset + latency?


            // start parallel cosmos running on new thread

            // set our version of those entities back to merged (delete spacetime),
            //   so they won't trigger another resolution
            //   (assuming the parallel context will finish correctly)
            // this will enable it to receive timestream updates again, 
            //   once future entity is resolved and starts propagating again
            
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