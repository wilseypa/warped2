#include <cstring>  // for std::memset
#include <fstream>

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
            case COAST_FORWARDED_EVENTS.value:
                sumReduceLocal(COAST_FORWARDED_EVENTS, coast_forward_events_by_node_);
                break;
            case TOTAL_EVENTS_RECEIVED.value:
                sumReduceLocal(TOTAL_EVENTS_RECEIVED, total_events_received_by_node_);
                break;
            case AVERAGE_MAX_MEMORY.value:
                global_stats_[AVERAGE_MAX_MEMORY] /= (1024*1024); // B to MB
                break;
            case GVT_CYCLES.value:
                global_stats_[GVT_CYCLES] = local_stats_[num_worker_threads_][GVT_CYCLES];
                break;
            case NUM_OBJECTS.value:
                sumReduceLocal(NUM_OBJECTS, num_objects_by_node_);
                break;
            case EVENTS_ROLLEDBACK.value:
                global_stats_[EVENTS_ROLLEDBACK] =
                    global_stats_[EVENTS_PROCESSED] -
                    global_stats_[EVENTS_COMMITTED];
                break;
            case AVERAGE_RBLENGTH.value:
                global_stats_[AVERAGE_RBLENGTH] = (global_stats_[TOTAL_ROLLBACKS] == 0) ? 0 :
                    (static_cast<double>(global_stats_[EVENTS_ROLLEDBACK]) /
                     (static_cast<double>(global_stats_[TOTAL_ROLLBACKS])));
                break;
            case EFFICIENCY.value:
                global_stats_[EFFICIENCY] =
                    static_cast<double>(global_stats_[EVENTS_COMMITTED]) /
                    static_cast<double>(global_stats_[EVENTS_PROCESSED]);
                break;
            case EVENTS_FOR_STARVED_OBJECTS.value:
                sumReduceLocal(EVENTS_FOR_STARVED_OBJECTS, starved_obj_events_by_node_);
                break;
            case SCHEDULED_EVENT_SWAPS_SUCCESS.value:
                sumReduceLocal(SCHEDULED_EVENT_SWAPS_SUCCESS, event_swaps_success_by_node_);
                break;
            case SCHEDULED_EVENT_SWAPS_FAILURE.value:
                sumReduceLocal(SCHEDULED_EVENT_SWAPS_FAILURE, event_swaps_failed_by_node_);
                break;
            case DESIGN_EFFICIENCY.value:
                auto scheduler_impact_cnt = global_stats_[EVENTS_FOR_STARVED_OBJECTS]
                                          + global_stats_[SCHEDULED_EVENT_SWAPS_SUCCESS]
                                          + global_stats_[SCHEDULED_EVENT_SWAPS_FAILURE];
                global_stats_[DESIGN_EFFICIENCY] =
                    1.0 - static_cast<double>(scheduler_impact_cnt) /
                            static_cast<double>(global_stats_[EVENTS_PROCESSED]);
                break;
            default:
                break;
        }
    }
}

void TimeWarpStatistics::writeToFile(double num_seconds) {
    if (stats_file_ == "none") {
        return;
    }

    std::ofstream ofs(stats_file_, std::ios::out | std::ios::app);

    for (unsigned int i = 0; i < comm_manager_->getNumProcesses(); i++) {
        ofs << i                                << ",\t"
                                                << ",\t"
            << num_objects_by_node_[i]          << ",\t"
            << local_pos_sent_by_node_[i]       << ",\t"
            << remote_pos_sent_by_node_[i]      << ",\t"
            << local_neg_sent_by_node_[i]       << ",\t"
            << remote_neg_sent_by_node_[i]      << ",\t"
            << primary_rollbacks_by_node_[i]    << ",\t"
            << secondary_rollbacks_by_node_[i]  << ",\t"
            << coast_forward_events_by_node_[i] << ",\t"
            << cancelled_events_by_node_[i]     << ",\t"
            << processed_events_by_node_[i]     << ",\t"
            << committed_events_by_node_[i]     << ",\t"
            << starved_obj_events_by_node_[i]   << ",\t"
            << event_swaps_success_by_node_[i]  << ",\t"
            << event_swaps_failed_by_node_[i]   << std::endl;
    }

    ofs << "Total"                                    << ",\t"
        << num_seconds                                << ",\t"
        << global_stats_[NUM_OBJECTS]                 << ",\t"
        << global_stats_[LOCAL_POSITIVE_EVENTS_SENT]  << ",\t"
        << global_stats_[REMOTE_POSITIVE_EVENTS_SENT] << ",\t"
        << global_stats_[LOCAL_NEGATIVE_EVENTS_SENT]  << ",\t"
        << global_stats_[REMOTE_NEGATIVE_EVENTS_SENT] << ",\t"
        << global_stats_[PRIMARY_ROLLBACKS]           << ",\t"
        << global_stats_[SECONDARY_ROLLBACKS]         << ",\t"
        << global_stats_[COAST_FORWARDED_EVENTS]      << ",\t"
        << global_stats_[CANCELLED_EVENTS]            << ",\t"
        << global_stats_[EVENTS_PROCESSED]            << ",\t"
        << global_stats_[EVENTS_COMMITTED]            << ",\t"
        << global_stats_[EVENTS_FOR_STARVED_OBJECTS]  << ",\t"
        << global_stats_[SCHEDULED_EVENT_SWAPS_SUCCESS] << ",\t"
        << global_stats_[SCHEDULED_EVENT_SWAPS_FAILURE] << ",\t"
        << global_stats_[AVERAGE_MAX_MEMORY]          << std::endl;

    ofs.close();
}

void TimeWarpStatistics::printStats() {

    std::cout << "Totals"                      << "\n"
              << "\tNumber of objects:         " << global_stats_[NUM_OBJECTS] << "\n\n"

              << "\tLocal events sent:         " << global_stats_[LOCAL_POSITIVE_EVENTS_SENT] << "\n"
              << "\tRemote events sent:        " << global_stats_[REMOTE_POSITIVE_EVENTS_SENT] << "\n"
              << "\tLocal anti-messages sent:  " << global_stats_[LOCAL_NEGATIVE_EVENTS_SENT] << "\n"
              << "\tRemote anti-messages sent: " << global_stats_[REMOTE_NEGATIVE_EVENTS_SENT] << "\n"
              << "\tTotal events sent:         " << global_stats_[TOTAL_EVENTS_SENT] << "\n"
              << "\tPercent remote:            " << global_stats_[PERCENT_REMOTE]*100.0 << "%\n\n"

              << "\tPrimary rollbacks:         " << global_stats_[PRIMARY_ROLLBACKS] << "\n"
              << "\tSecondary rollbacks:       " << global_stats_[SECONDARY_ROLLBACKS] << "\n"
              << "\tTotal Rollbacks:           " << global_stats_[TOTAL_ROLLBACKS] << "\n\n"

              << "\tTotal anti-messages sent:  " << global_stats_[TOTAL_NEGATIVE_EVENTS] << "\n"
              << "\tCancelled events:          " << global_stats_[CANCELLED_EVENTS] << "\n\n"

              << "\tCoast forward events:      " << global_stats_[COAST_FORWARDED_EVENTS] << "\n\n"

              << "\tTotal events processed:    " << global_stats_[EVENTS_PROCESSED] << "\n"
              << "\tTotal events committed:    " << global_stats_[EVENTS_COMMITTED] << "\n"
              << "\tTotal events rolled back:  " << global_stats_[EVENTS_ROLLEDBACK] << "\n\n"

              << "\tAverage rollback length:   " << global_stats_[AVERAGE_RBLENGTH] << "\n"
              << "\tEfficiency:                " << global_stats_[EFFICIENCY]*100.0 << "%\n\n"

              << "\tEvents for starved objs:   " << global_stats_[EVENTS_FOR_STARVED_OBJECTS] << "\n"
              << "\tSched event swaps success: " << global_stats_[SCHEDULED_EVENT_SWAPS_SUCCESS] << "\n"
              << "\tSched event swaps failure: " << global_stats_[SCHEDULED_EVENT_SWAPS_FAILURE] << "\n"
              << "\tDesign Efficiency:         " << global_stats_[DESIGN_EFFICIENCY]*100.0 << "%\n\n"

              << "\tAverage maximum memory:    " << global_stats_[AVERAGE_MAX_MEMORY] << " MB\n"
              << "\tGVT cycles:                " << global_stats_[GVT_CYCLES] << std::endl << std::endl;

    delete [] local_pos_sent_by_node_;
    delete [] local_neg_sent_by_node_;
    delete [] remote_pos_sent_by_node_;
    delete [] remote_neg_sent_by_node_;
    delete [] primary_rollbacks_by_node_;
    delete [] secondary_rollbacks_by_node_;
    delete [] coast_forward_events_by_node_;
    delete [] cancelled_events_by_node_;
    delete [] total_events_received_by_node_;
    delete [] num_objects_by_node_;
    delete [] processed_events_by_node_;
    delete [] committed_events_by_node_;
    delete [] starved_obj_events_by_node_;
    delete [] event_swaps_success_by_node_;
    delete [] event_swaps_failed_by_node_;
}

} // namespace warped


