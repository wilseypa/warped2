#include <cstring>  // for std::memset

#include "TimeWarpStatistics.hpp"
#include "utility/memory.hpp"

namespace warped {

void TimeWarpStatistics::initialize(unsigned int num_worker_threads, unsigned int num_objects) {
    num_worker_threads_ = num_worker_threads;

    local_stats_ = make_unique<Stats []>(num_worker_threads+1);
    local_stats_[num_worker_threads][NUM_OBJECTS] = num_objects;
}

void TimeWarpStatistics::calculateStats() {
    for (unsigned int i = LOCAL_POSITIVE_EVENTS_SENT.value; i < NUM_STATISTICS.value; i++) {
        switch (i) {
            case LOCAL_POSITIVE_EVENTS_SENT.value:
                sumReduceUint64(LOCAL_POSITIVE_EVENTS_SENT);
                break;
            case REMOTE_POSITIVE_EVENTS_SENT.value:
                sumReduceUint64(REMOTE_POSITIVE_EVENTS_SENT);
                break;
            case LOCAL_NEGATIVE_EVENTS_SENT.value:
                sumReduceUint64(LOCAL_NEGATIVE_EVENTS_SENT);
                break;
            case REMOTE_NEGATIVE_EVENTS_SENT.value:
                sumReduceUint64(REMOTE_NEGATIVE_EVENTS_SENT);
                break;
            case TOTAL_EVENTS_SENT.value:
                global_stats_[TOTAL_EVENTS_SENT] =
                    global_stats_[LOCAL_POSITIVE_EVENTS_SENT] +
                    global_stats_[REMOTE_POSITIVE_EVENTS_SENT] +
                    global_stats_[LOCAL_NEGATIVE_EVENTS_SENT] +
                    global_stats_[REMOTE_NEGATIVE_EVENTS_SENT];
                break;
            case PERCENT_REMOTE.value:
                global_stats_[PERCENT_REMOTE] =
                    static_cast<double>(global_stats_[REMOTE_POSITIVE_EVENTS_SENT] +
                                        global_stats_[REMOTE_NEGATIVE_EVENTS_SENT]) /
                    static_cast<double>(global_stats_[TOTAL_EVENTS_SENT]);
                break;
            case PRIMARY_ROLLBACKS.value:
                sumReduceUint64(PRIMARY_ROLLBACKS);
                break;
            case SECONDARY_ROLLBACKS.value:
                sumReduceUint64(SECONDARY_ROLLBACKS);
                break;
            case TOTAL_ROLLBACKS.value:
                global_stats_[TOTAL_ROLLBACKS] =
                    global_stats_[PRIMARY_ROLLBACKS] +
                    global_stats_[SECONDARY_ROLLBACKS];
                break;
            case TOTAL_EVENTS_RECEIVED.value:
                sumReduceUint64(TOTAL_EVENTS_RECEIVED);
                break;
            case EVENTS_PROCESSED.value:
                sumReduceUint64(EVENTS_PROCESSED);
                break;
            case EVENTS_COMMITTED.value:
                sumReduceUint64(EVENTS_COMMITTED);
                break;
            case TOTAL_NEGATIVE_EVENTS.value:
                global_stats_[TOTAL_NEGATIVE_EVENTS] =
                    global_stats_[LOCAL_NEGATIVE_EVENTS_SENT] +
                    global_stats_[REMOTE_NEGATIVE_EVENTS_SENT];
                break;
            case CANCELLED_EVENTS.value:
                sumReduceUint64(CANCELLED_EVENTS);
                break;
            case GVT_CYCLES.value:
                global_stats_[GVT_CYCLES] = local_stats_[num_worker_threads_][GVT_CYCLES];
                break;
            case NUM_OBJECTS.value:
                sumReduceUint64(NUM_OBJECTS);
            default:
                break;
        }
    }
}

void TimeWarpStatistics::printStats() {
    std::cout << "Number of objects:         " << global_stats_[NUM_OBJECTS] << "\n\n"

              << "Local events sent:         " << global_stats_[LOCAL_POSITIVE_EVENTS_SENT] << "\n"
              << "Remote events sent:        " << global_stats_[REMOTE_POSITIVE_EVENTS_SENT] << "\n"
              << "Local anti-messages sent:  " << global_stats_[LOCAL_NEGATIVE_EVENTS_SENT] << "\n"
              << "Remote anti-messages sent: " << global_stats_[REMOTE_NEGATIVE_EVENTS_SENT] << "\n"
              << "Percent remote:            " << global_stats_[PERCENT_REMOTE]*100.0 << "%\n\n"

              << "Primary rollbacks:         " << global_stats_[PRIMARY_ROLLBACKS] << "\n"
              << "Secondary rollbacks:       " << global_stats_[SECONDARY_ROLLBACKS] << "\n"
              << "Total Rollbacks:           " << global_stats_[TOTAL_ROLLBACKS] << "\n\n"

              << "Total anti-messages sent:  " << global_stats_[TOTAL_NEGATIVE_EVENTS] << "\n"
              << "Cancelled events:          " << global_stats_[CANCELLED_EVENTS] << "\n\n"

              << "Total events sent:         " << global_stats_[TOTAL_EVENTS_SENT] << "\n"
              << "Total events recevied:     " << global_stats_[TOTAL_EVENTS_RECEIVED] << "\n\n"

              << "Total events processed:    " << global_stats_[EVENTS_PROCESSED] << "\n"
              << "Total events committed:    " << global_stats_[EVENTS_COMMITTED] << "\n\n"

              << "GVT cycles:                " << global_stats_[GVT_CYCLES] << "\n"
              << std::endl;
}

} // namespace warped

