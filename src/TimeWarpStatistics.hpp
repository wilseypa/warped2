#ifndef TIME_WARP_STATISTICS_HPP
#define TIME_WARP_STATISTICS_HPP

#include <memory>   // for unique_ptr
#include <cstdint>  // uint64_t
#include <tuple>

#include "TimeWarpCommunicationManager.hpp"

namespace warped {

template <unsigned I>
struct counter_index{
    counter_index(){}
    static unsigned const value = I;
};

struct Counters {

    std::tuple<
        uint64_t,                   // Local positive events sent   0
        uint64_t,                   // Remote positive events sent  1
        uint64_t,                   // Local negative events sent   2
        uint64_t,                   // Remote positive events sent  3
        uint64_t,                   // Primary rollbacks            4
        uint64_t,                   // Secondary rollbacks          5
        uint64_t,                   // Coast forward events         6
        uint64_t,                   // Events processed             7
        uint64_t,                   // Events committed             8
        uint64_t,                   // Cancelled events             9
        uint64_t,                   // Total Events Received        10
        uint64_t,                   // Number of objects            11
        uint64_t,                   // Events for starved objects   12
        uint64_t,                   // Scheduled event swap success 13
        uint64_t,                   // Scheduled event swap failed  14
    > counters_;

    template<unsigned I>
    auto operator[](counter_index<I>) -> decltype(std::get<I>(counters_)) {
        return std::get<I>(counters_);
    }
};

const counter_index<0>  LOCAL_POSITIVE_EVENTS_SENT;
const counter_index<1>  REMOTE_POSITIVE_EVENTS_SENT;
const counter_index<2>  LOCAL_NEGATIVE_EVENTS_SENT;
const counter_index<3>  REMOTE_NEGATIVE_EVENTS_SENT;
const counter_index<4>  PRIMARY_ROLLBACKS;
const counter_index<5>  SECONDARY_ROLLBACKS;
const counter_index<6>  COAST_FORWARDED_EVENTS;
const counter_index<7>  EVENTS_PROCESSED;
const counter_index<8>  EVENTS_COMMITTED;
const counter_index<9>  CANCELLED_EVENTS;
const counter_index<10> TOTAL_EVENTS_RECEIVED;
const counter_index<11> NUM_OBJECTS;
const counter_index<12> EVENTS_FOR_STARVED_OBJECTS;
const counter_index<13> SCHEDULED_EVENT_SWAPS_SUCCESS;
const counter_index<14> SCHEDULED_EVENT_SWAPS_FAILURE;

template <unsigned I>
struct derived_index{
    derived_index(){}
    static unsigned const value = I;
};

struct Derived {

    std::tuple<
        uint64_t,                   // Total events sent            0
        double,                     // Percent remote events        1
        uint64_t,                   // Total rollbacks              2
        uint64_t,                   // Total negative events sent   3
        uint64_t,                   // Average Maximum Memory       4
        uint64_t,                   // GVT cycles                   5
        uint64_t,                   // Rolled back events           6
        double,                     // Average Rollback Length      7
        double,                     // Efficiency                   8
        double,                     // Design Efficiency            9
    > derived_;

    template<unsigned I>
    auto operator[](derived_index<I>) -> decltype(std::get<I>(derived_)) {
        return std::get<I>(derived_);
};

const derived_index<0>  TOTAL_EVENTS_SENT;
const derived_index<1>  PERCENT_REMOTE;
const derived_index<2>  TOTAL_ROLLBACKS;
const derived_index<3>  TOTAL_NEGATIVE_EVENTS;
const derived_index<4>  AVERAGE_MAX_MEMORY;
const derived_index<5>  GVT_CYCLES;
const derived_index<6>  EVENTS_ROLLEDBACK;
const derived_index<7>  AVERAGE_RBLENGTH;
const derived_index<8>  EFFICIENCY;
const derived_index<9>  DESIGN_EFFICIENCY;


class TimeWarpStatistics {
public:
    TimeWarpStatistics(std::shared_ptr<TimeWarpCommunicationManager> comm_manager,
        std::string stats_file) :
        comm_manager_(comm_manager), stats_file_(stats_file) {}

    void initialize(unsigned int num_worker_threads, unsigned int num_objects);

    template <unsigned I>
    void updateAverage(derived_index<I> i, uint64_t new_val, unsigned int count) {
        global_derived_[i] = (new_val + (count - 1) * global_derived_[i]) / (count);
    }

    template <unsigned I>
    uint64_t upCount(counter_index<I> i, unsigned int thread_id, unsigned int num = 1) {
        auto c = prime_;
        c[thread_id][i] += num;
        return c[thread_id][i];
    }

    template <unsigned I>
    void sumReduceLocal(counter_index<I> j, uint64_t *&recv_array) {
        uint64_t local_count = 0;

        for (unsigned int i = 0; i < num_worker_threads_+1; i++) {
            local_count += prime_[i][j] + reserve_[i][j] + offline_[i][j];
        }

        recv_array = new uint64_t[comm_manager_->getNumProcesses()];
        comm_manager_->gatherUint64(&local_count, recv_array);

        for (unsigned int i = 0; i < comm_manager_->getNumProcesses(); i++) {
            global_counters_[j] += recv_array[i];
        }
    }

    void calculateStats();

    void writeToFile(double num_seconds);

    void printStats();

    void rotate();

private:

    std::unique_ptr<Counters []> prime_;
    std::unique_ptr<Counters []> reserve_;
    std::unique_ptr<Counters []> offline_;

    Counters global_counters_;
    Derived  global_derived_;

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
    uint64_t *event_swaps_success_by_node_;
    uint64_t *event_swaps_failed_by_node_;

    std::shared_ptr<TimeWarpCommunicationManager> comm_manager_;

    std::string stats_file_;

    unsigned int num_worker_threads_;

};

} // namespace warped

#endif
