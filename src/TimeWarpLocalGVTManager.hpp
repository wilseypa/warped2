#ifndef LOCAL_GVT_MANAGER
#define LOCAL_GVT_MANAGER

#include <mutex>
#include <atomic>
#include <memory>

namespace warped {

class TimeWarpLocalGVTManager {
public:
    TimeWarpLocalGVTManager() = default;

    void initialize(unsigned int num_worker_threads, unsigned int num_local_objects);

    bool startGVT();
    bool completeGVT();

    void resetState();

    void receiveEventUpdateState(unsigned int timestamp, unsigned int thread_id);

    void sendEventUpdateState(unsigned int timestamp, unsigned int thread_id);

    void reportStraggler(unsigned int local_object_id);
    void reportRollback(unsigned int local_object_id);

    unsigned int getGVT() { return gVT_; }

protected:
    unsigned int getMinimumLVT();

private:
    unsigned int gVT_ = 0;

    std::atomic<unsigned int> min_lvt_flag_ = ATOMIC_VAR_INIT(0);

    // local min lvt of each worker thread
    std::unique_ptr<unsigned int []> min_lvt_;

    // minimum timestamp of sent events used for minimum lvt calculation
    std::unique_ptr<unsigned int []> send_min_;

    // flag to determine if worker thread has already calculated min lvt
    std::unique_ptr<bool []> calculated_min_flag_;

    unsigned int num_worker_threads_;

    unsigned int num_local_objects_;

    bool started_local_gvt_ = false;

    std::mutex straggler_flags_lock_;

    std::unique_ptr<unsigned int []> continuous_straggler_flags_;
    unsigned int continuous_straggler_count_ = 0;

    std::unique_ptr<unsigned int []> local_gvt_straggler_flags_;
    unsigned int local_gvt_straggler_count_ = 0;
};

} // namespace warped

#endif
