#ifndef SYNCHRONOUS_GVT_MANAGER_HPP
#define SYNCHRONOUS_GVT_MANAGER_HPP

#include <memory> // for shared_ptr
#include <atomic>
#include <mutex>

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

    void progressGVT(unsigned int &local_gvt_passed_in) override;

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

    void setGVTEstCycle(bool cycleStatus) override;

    void workerThreadGVTBarrierSync() override;

    unsigned int getPrevGVT() override;

    void setPrevGVT(unsigned int prev_GVT) override;

    void setGVT(unsigned int new_GVT) override;

protected:
    bool gvt_updated_ = false;

    std::atomic<int64_t> white_msg_count_ = ATOMIC_VAR_INIT(0);

    std::atomic<Color> color_ = ATOMIC_VAR_INIT(Color::WHITE);

    std::atomic<unsigned int> local_gvt_flag_ = ATOMIC_VAR_INIT(0);

    std::unique_ptr<unsigned int []> local_min_;

    std::unique_ptr<unsigned int []> send_min_;

    unsigned int recv_min_;

    pthread_barrier_t min_report_barrier_;

    std::mutex gvt_est_cycle_lock_;

    std::mutex prev_gvt_lock_;

    std::mutex gvt_lock_;

};

} // warped namespace

#endif
