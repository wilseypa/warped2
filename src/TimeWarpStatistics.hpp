#ifndef TIME_WARP_STATISTICS_HPP
#define TIME_WARP_STATISTICS_HPP

#include <memory>   // for unique_ptr
#include <cstdint>  // uint64_t
#include <tuple>

#include "TimeWarpCommunicationManager.hpp"

namespace warped {

template <unsigned I>
struct stats_index{
    stats_index(){}
    static unsigned const value = I;
};

struct Stats {

    std::tuple<
        uint64_t,                   // Local positive events sent   0
        uint64_t,                   // Remote positive events sent  1
        uint64_t,                   // Local negative events sent   2
        uint64_t,                   // Remote positive events sent  3
        uint64_t,                   // Total events sent            4
        double,                     // Percent remote events        5
        uint64_t,                   // Primary rollbacks            6
        uint64_t,                   // Secondary rollbacks          7
        uint64_t,                   // Total rollbacks              8
        uint64_t,                   // Coast forward events         9
        uint64_t,                   // Events processed             10
        uint64_t,                   // Events committed             11
        uint64_t,                   // Total negative events sent   12
        uint64_t,                   // Cancelled events             13
        uint64_t,                   // Total Events Received        14
        uint64_t,                   // Average Maximum Memory       15
        uint64_t,                   // GVT cycles                   16
        uint64_t,                   // Number of objects            17
        uint64_t,                   // Rolled back events           18
        double,                     // Average Rollback Length      19
        double,                     // Efficiency                   20
        uint64_t,                   // Events for starved objects   21
        uint64_t,                   // Scheduled event swaps        22
        double,                     // Design Efficiency            23
        uint64_t                    // dummy/number of elements     24
    > stats_;

    template<unsigned I>
    auto operator[](stats_index<I>) -> decltype(std::get<I>(stats_)) {
        return std::get<I>(stats_);
    }

};

const stats_index<0> LOCAL_POSITIVE_EVENTS_SENT;
const stats_index<1> REMOTE_POSITIVE_EVENTS_SENT;
const stats_index<2> LOCAL_NEGATIVE_EVENTS_SENT;
const stats_index<3> REMOTE_NEGATIVE_EVENTS_SENT;
const stats_index<4> TOTAL_EVENTS_SENT;
const stats_index<5> PERCENT_REMOTE;
const stats_index<6> PRIMARY_ROLLBACKS;
const stats_index<7> SECONDARY_ROLLBACKS;
const stats_index<8> TOTAL_ROLLBACKS;
const stats_index<9> COAST_FORWARDED_EVENTS;
const stats_index<10> EVENTS_PROCESSED;
const stats_index<11> EVENTS_COMMITTED;
const stats_index<12> TOTAL_NEGATIVE_EVENTS;
const stats_index<13> CANCELLED_EVENTS;
const stats_index<14> TOTAL_EVENTS_RECEIVED;
const stats_index<15> AVERAGE_MAX_MEMORY;
const stats_index<16> GVT_CYCLES;
const stats_index<17> NUM_OBJECTS;
const stats_index<18> EVENTS_ROLLEDBACK;
const stats_index<19> AVERAGE_RBLENGTH;
const stats_index<20> EFFICIENCY;
const stats_index<21> EVENTS_FOR_STARVED_OBJECTS;
const stats_index<22> SCHEDULED_EVENT_SWAPS;
const stats_index<23> DESIGN_EFFICIENCY;
const stats_index<24> NUM_STATISTICS;

class TimeWarpStatistics {
public:
    TimeWarpStatistics(std::shared_ptr<TimeWarpCommunicationManager> comm_manager,
        std::string stats_file) :
        comm_manager_(comm_manager), stats_file_(stats_file) {}

    void initialize(unsigned int num_worker_threads, unsigned int num_objects);

    template <unsigned I>
    void updateAverage(stats_index<I> i, uint64_t new_val, unsigned int count) {
        global_stats_[i] = (new_val + (count - 1) * global_stats_[i]) / (count);
    }

    template <unsigned I>
    uint64_t upCount(stats_index<I> i, unsigned int thread_id, unsigned int num = 1) {
        local_stats_[thread_id][i] += num;
        return local_stats_[thread_id][i];
    }

    template <unsigned I>
    void sumReduceLocal(stats_index<I> j, uint64_t *&recv_array) {
        uint64_t local_count = 0;

        for (unsigned int i = 0; i < num_worker_threads_+1; i++) {
            local_count += local_stats_[i][j];
        }

        recv_array = new uint64_t[comm_manager_->getNumProcesses()];
        comm_manager_->gatherUint64(&local_count, recv_array);

        for (unsigned int i = 0; i < comm_manager_->getNumProcesses(); i++) {
            global_stats_[j] += recv_array[i];
        }
    }

    void calculateStats();

    void writeToFile(double num_seconds);

    void printStats();

private:

    std::unique_ptr<Stats []> local_stats_;
    Stats global_stats_;

    uint64_t *local_pos_sent_by_node_;
    uint64_t *local_neg_sent_by_node_;
    uint64_t *remote_pos_sent_by_node_;
    uint64_t *remote_neg_sent_by_node_;
    uint64_t *primary_rollbacks_by_node_;
    uint64_t *secondary_rollbacks_by_node_;
    uint64_t *coast_forward_events_by_node_;
    uint64_t *cancelled_events_by_node_;
    uint64_t *processed_events_by_node_;
    uint64_t *committed_events_by_node_;
    uint64_t *total_events_received_by_node_;
    uint64_t *num_objects_by_node_;
    uint64_t *starved_obj_events_by_node_;
    uint64_t *event_swaps_by_node_;

    std::shared_ptr<TimeWarpCommunicationManager> comm_manager_;

    std::string stats_file_;

    unsigned int num_worker_threads_;

};

} // namespace warped

#endif
