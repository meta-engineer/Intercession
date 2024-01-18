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
            if (!is_divergent(data.spacetime.timestreamState)) return;
            // thus we can assume its future is in a superposition

            // check coherency timing to promote forking to forked after minimum threshold
            if (data.spacetime.timestreamStateCoherency + FORKED_THRESHOLD_MIN <= m_lastCoherency)
            {
                data.spacetime.timestreamState = TimestreamState::forked;
            }

            // check to trigger resolution after maximum threshold
            if (!m_resolutionNeeded && data.spacetime.timestreamStateCoherency + FORKED_THRESHOLD_MAX <= m_lastCoherency)
            {
                // (if they pass this check they are necessarily in forked state)
                assert(data.spacetime.timestreamState == TimestreamState::forked);
                PLEEPLOG_DEBUG("Entity " + std::to_string(data.entity) + " is due for resolution at: " + std::to_string(m_lastCoherency));
                m_resolutionNeeded = true;
            }
        }

        void engage(double deltaTime, std::weak_ptr<Cosmos> localCosmos, TimelineApi* localTimelineApi)
        {
            UNREFERENCED_PARAMETER(deltaTime);

            std::shared_ptr<Cosmos> cosmos = localCosmos.lock();
            if (localCosmos.expired()) return;

            // store coherency for next frame
            m_lastCoherency = cosmos->get_coherency();

            // signal to parallel context that resolution is needed
            // (this is idempotent)
            if (m_resolutionNeeded) localTimelineApi->parallel_notify_divergence();

            /// TODO: feature for updating superpositions as forks happen inside parallel
            /// We would need a timeline api method to fetch newly divergent entities...
            /// ACTUALLY parallel can send updates to server as they happen, via timelineApi

            /// If parallel is simulating our recent past, we need to continually feed it new target coherencies (m_lastCoherency + 1) until FINISHED event is sent, and it moves onto the next
            if (localTimelineApi->parallel_get_timeslice() == cosmos->get_host_id() + 1U)
            {
                localTimelineApi->parallel_start(m_lastCoherency + 1);
            }
        }

        void clear()
        {
            m_resolutionNeeded = false;
            // keep m_lastCoherency for next frame's submittions
        }

    protected:
        // note if a submitted entity candidate is ready for resolution
        bool m_resolutionNeeded = false;

        // store coherency at previous frame to check submittions for next frame
        uint16_t m_lastCoherency = 0;
    };
}

#endif // SUPERPOSITION_RELAY_H