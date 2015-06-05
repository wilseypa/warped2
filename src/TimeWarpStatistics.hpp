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
        uint64_t,                   // Total events received        9
        uint64_t,                   // Events processed             10
        uint64_t,                   // Events committed             11
        uint64_t,                   // Total negative events sent   12
        uint64_t,                   // Cancelled events             13
        uint64_t,                   // GVT cycles                   14
        uint64_t,                   // Number of objects            15
        uint64_t                    // dummy/number of elements     16
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
const stats_index<9> TOTAL_EVENTS_RECEIVED;
const stats_index<10> EVENTS_PROCESSED;
const stats_index<11> EVENTS_COMMITTED;
const stats_index<12> TOTAL_NEGATIVE_EVENTS;
const stats_index<13> CANCELLED_EVENTS;
const stats_index<14> GVT_CYCLES;
const stats_index<15> NUM_OBJECTS;
const stats_index<16> NUM_STATISTICS;

class TimeWarpStatistics {
public:
    TimeWarpStatistics(std::shared_ptr<TimeWarpCommunicationManager> comm_manager) :
        comm_manager_(comm_manager) {}

    void initialize(unsigned int num_worker_threads, unsigned int num_objects);

    template <unsigned I>
    void upCount(stats_index<I> i, unsigned int thread_id, unsigned int num = 1) {
        local_stats_[thread_id][i] += num;
    }

    template <unsigned I>
    void sumReduceUint64(stats_index<I> j) {
        uint64_t local_count = 0;

        for (unsigned int i = 0; i < num_worker_threads_+1; i++) {
            local_count += local_stats_[i][j];
        }

        comm_manager_->sumReduceUint64(&local_count, &global_stats_[j]);
    }

    void calculateStats();

    void printStats();

private:

    std::unique_ptr<Stats []> local_stats_;

    Stats global_stats_;

    std::shared_ptr<TimeWarpCommunicationManager> comm_manager_;

    unsigned int num_worker_threads_;

};

} // namespace warped

#endif

