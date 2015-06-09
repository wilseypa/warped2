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
                sumReduceLocal(LOCAL_POSITIVE_EVENTS_SENT, local_pos_sent_by_node_);
                break;
            case REMOTE_POSITIVE_EVENTS_SENT.value:
                sumReduceLocal(REMOTE_POSITIVE_EVENTS_SENT, remote_pos_sent_by_node_);
                break;
            case LOCAL_NEGATIVE_EVENTS_SENT.value:
                sumReduceLocal(LOCAL_NEGATIVE_EVENTS_SENT, local_neg_sent_by_node_);
                break;
            case REMOTE_NEGATIVE_EVENTS_SENT.value:
                sumReduceLocal(REMOTE_NEGATIVE_EVENTS_SENT, remote_neg_sent_by_node_);
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
                sumReduceLocal(PRIMARY_ROLLBACKS, primary_rollbacks_by_node_);
                break;
            case SECONDARY_ROLLBACKS.value:
                sumReduceLocal(SECONDARY_ROLLBACKS, secondary_rollbacks_by_node_);
                break;
            case TOTAL_ROLLBACKS.value:
                global_stats_[TOTAL_ROLLBACKS] =
                    global_stats_[PRIMARY_ROLLBACKS] +
                    global_stats_[SECONDARY_ROLLBACKS];
                break;
            case EVENTS_PROCESSED.value:
                sumReduceLocal(EVENTS_PROCESSED, processed_events_by_node_);
                break;
            case EVENTS_COMMITTED.value:
                sumReduceLocal(EVENTS_COMMITTED, committed_events_by_node_);
                break;
            case TOTAL_NEGATIVE_EVENTS.value:
                global_stats_[TOTAL_NEGATIVE_EVENTS] =
                    global_stats_[LOCAL_NEGATIVE_EVENTS_SENT] +
                    global_stats_[REMOTE_NEGATIVE_EVENTS_SENT];
                break;
            case CANCELLED_EVENTS.value:
                sumReduceLocal(CANCELLED_EVENTS, cancelled_events_by_node_);
                break;
            case GVT_CYCLES.value:
                global_stats_[GVT_CYCLES] = local_stats_[num_worker_threads_][GVT_CYCLES];
                break;
            case NUM_OBJECTS.value:
                sumReduceLocal(NUM_OBJECTS, num_objects_by_node_);
            default:
                break;
        }
    }
}

void TimeWarpStatistics::printStats() {

    for (unsigned int i = 0; i < comm_manager_->getNumProcesses(); i++) {
        std::cout << "Node "                      << i                              << "\n"
                  << "\tNumber of objects:        " << num_objects_by_node_[i]        << "\n\n"

                  << "\tLocal events sent:        " << local_pos_sent_by_node_[i]     << "\n"
                  << "\tRemote events sent:       " << remote_pos_sent_by_node_[i]    << "\n"
                  << "\tLocal anti-messages sent: " << local_neg_sent_by_node_[i]     << "\n"
                  << "\tRemote anti-messages sent:" << remote_neg_sent_by_node_[i]   << "\n\n"

                  << "\tPrimary rollbacks:        " << primary_rollbacks_by_node_[i]  << "\n"
                  << "\tSecondary rollbacks:      " << secondary_rollbacks_by_node_[i] << "\n\n"

                  << "\tCancelled events:         " << cancelled_events_by_node_[i]    << "\n\n"

                  << "\tEvents processed:         " << processed_events_by_node_[i]   << "\n"
                  << "\tEvents committed:         " << committed_events_by_node_[i]   << std::endl << std::endl;
    }

    std::cout << "Totals"                      << "\n"
              << "\tNumber of objects:         " << global_stats_[NUM_OBJECTS] << "\n"
              << "\tLocal events sent:         " << global_stats_[LOCAL_POSITIVE_EVENTS_SENT] << "\n"
              << "\tRemote events sent:        " << global_stats_[REMOTE_POSITIVE_EVENTS_SENT] << "\n"
              << "\tLocal anti-messages sent:  " << global_stats_[LOCAL_NEGATIVE_EVENTS_SENT] << "\n"
              << "\tRemote anti-messages sent: " << global_stats_[REMOTE_NEGATIVE_EVENTS_SENT] << "\n"
              << "\tPercent remote:            " << global_stats_[PERCENT_REMOTE]*100.0 << "%\n\n"

              << "\tPrimary rollbacks:         " << global_stats_[PRIMARY_ROLLBACKS] << "\n"
              << "\tSecondary rollbacks:       " << global_stats_[SECONDARY_ROLLBACKS] << "\n"
              << "\tTotal Rollbacks:           " << global_stats_[TOTAL_ROLLBACKS] << "\n\n"

              << "\tTotal anti-messages sent:  " << global_stats_[TOTAL_NEGATIVE_EVENTS] << "\n"
              << "\tCancelled events:          " << global_stats_[CANCELLED_EVENTS] << "\n\n"

              << "\tTotal events sent:         " << global_stats_[TOTAL_EVENTS_SENT] << "\n"

              << "\tTotal events processed:    " << global_stats_[EVENTS_PROCESSED] << "\n"
              << "\tTotal events committed:    " << global_stats_[EVENTS_COMMITTED] << "\n\n"

              << "\tGVT cycles:                " << global_stats_[GVT_CYCLES] << std::endl << std::endl;


    delete [] local_pos_sent_by_node_;
    delete [] local_neg_sent_by_node_;
    delete [] remote_pos_sent_by_node_;
    delete [] remote_neg_sent_by_node_;
    delete [] primary_rollbacks_by_node_;
    delete [] secondary_rollbacks_by_node_;
    delete [] cancelled_events_by_node_;
    delete [] num_objects_by_node_;
    delete [] processed_events_by_node_;
    delete [] committed_events_by_node_;
}

} // namespace warped


