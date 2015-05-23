#ifndef LOCAL_GVT_MANAGER
#define LOCAL_GVT_MANAGER

#include <mutex>
#include <atomic>
#include <memory>

namespace warped {

class TimeWarpLocalGVTManager {
public:
    TimeWarpLocalGVTManager() = default;

    void initialize(unsigned int num_worker_threads);

    // Check for start of a local GVT calculation
    bool startGVT();

    // Check for completion of local GVT calculation
    bool completeGVT();

    // Reset for new GVT calculation
    void resetState();

    // Report timestamp of received event if we are in the middle of a GVT calculation.
    void receiveEventUpdateState(unsigned int timestamp, unsigned int thread_id,
        unsigned int local_gvt_flag);

    // Report send timestamp if we are in the middle of a GVT calculation
    void sendEventUpdateState(unsigned int timestamp, unsigned int thread_id);

    // Get GVT
    unsigned int getGVT() { return gVT_; }

    // Get flag which indicated number of worker threads that still need to report a minimum
    unsigned int getLocalGVTFlag();

protected:
    // Reduces minimums from all threads
    unsigned int getMinimumLVT();

private:
    // GVT of this only this node
    unsigned int gVT_ = 0;

    std::atomic<unsigned int> local_gvt_flag_ = ATOMIC_VAR_INIT(0);

    // local minimum of each object including unprocessed events and sent events
    std::unique_ptr<unsigned int []> local_min_;

    // minimum timestamp of sent events used for minimum local min calculation
    std::unique_ptr<unsigned int []> send_min_;

    // flag to determine if object has already calculated min
    std::unique_ptr<bool []> calculated_min_flag_;

    unsigned int num_worker_threads_;

    bool started_local_gvt_ = false;
};

} // namespace warped

#endif
