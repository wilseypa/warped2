#ifndef SYNCHRONOUS_GVT_MANAGER_HPP
#define SYNCHRONOUS_GVT_MANAGER_HPP

#include <memory> // for shared_ptr
#include <atomic>
#include <shared_mutex>

#include <pthread.h>

#include "TimeWarpEventDispatcher.hpp"
#include "TimeWarpGVTManager.hpp"

namespace warped {

class TimeWarpSynchronousGVTManager : public TimeWarpGVTManager {
public:
    TimeWarpSynchronousGVTManager(std::shared_ptr<TimeWarpCommunicationManager> comm_manager,
        unsigned int period, unsigned int num_worker_threads)
        : TimeWarpGVTManager(comm_manager, period, num_worker_threads) {}

    virtual ~TimeWarpSynchronousGVTManager() = default;

    void initialize() override;

    bool readyToStart()  override;

    void progressGVT() override;

    void receiveEventUpdate(std::shared_ptr<Event>& event, Color color) override;

    Color sendEventUpdate(std::shared_ptr<Event>& event) override;

    bool gvtUpdated() override;

    inline int64_t getMessageCount() override {
        return white_msg_count_.load();
    }

    void reportThreadMin(unsigned int timestamp, unsigned int thread_id,
                                 unsigned int local_gvt_flag) override;

    void reportThreadSendMin(unsigned int timestamp, unsigned int thread_id) override;

    unsigned int getLocalGVTFlag() override;

protected:
    bool gvt_updated_ = false;

    std::atomic<int64_t> white_msg_count_ = ATOMIC_VAR_INIT(0);

    std::atomic<Color> color_ = ATOMIC_VAR_INIT(Color::WHITE);

    std::atomic<unsigned int> local_gvt_flag_ = ATOMIC_VAR_INIT(0);

    std::shared_mutex report_gvt_lock_;

    bool report_gvt_;

    std::unique_ptr<unsigned int []> local_min_;

    std::unique_ptr<unsigned int []> send_min_;

    unsigned int recv_min_;

    pthread_barrier_t min_report_barrier_;

};

} // warped namespace

#endif
