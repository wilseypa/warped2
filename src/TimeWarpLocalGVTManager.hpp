#ifndef LOCAL_GVT_MANAGER
#define LOCAL_GVT_MANAGER

#include <mutex>
#include <atomic>
#include <memory>

namespace warped {

class TimeWarpLocalGVTManager {
public:
    TimeWarpLocalGVTManager() = default;

    void initialize(unsigned int num_local_objects);

    bool startGVT();
    bool completeGVT();

    void resetState();

    void receiveEventUpdateState(unsigned int timestamp, unsigned int local_object_id,
        unsigned int local_gvt_flag);

    void sendEventUpdateState(unsigned int timestamp, unsigned int local_object_id);

    unsigned int getGVT() { return gVT_; }

    unsigned int getLocalGVTFlag();

protected:
    unsigned int getMinimumLVT();

private:
    unsigned int gVT_ = 0;

    std::atomic<unsigned int> local_gvt_flag_ = ATOMIC_VAR_INIT(0);

    // local minimum of each object including unprocessed events and sent events
    std::unique_ptr<unsigned int []> local_min_;

    // minimum timestamp of sent events used for minimum local min calculation
    std::unique_ptr<unsigned int []> send_min_;

    // flag to determine if object has already calculated min
    std::unique_ptr<bool []> calculated_min_flag_;

    unsigned int num_local_objects_;

    bool started_local_gvt_ = false;
};

} // namespace warped

#endif
