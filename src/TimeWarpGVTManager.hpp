#ifndef GVT_MANAGER_HPP
#define GVT_MANAGER_HPP

#include <memory> // for shared_ptr
#include <mutex>

#include "TimeWarpEventDispatcher.hpp"
#include "TimeWarpKernelMessage.hpp"

namespace warped {

enum class GVTState { IDLE, LOCAL, GLOBAL };
enum class Color { WHITE, RED };

class TimeWarpGVTManager {
public:
    TimeWarpGVTManager(std::shared_ptr<TimeWarpCommunicationManager> comm_manager, unsigned int period,
        unsigned int num_worker_threads)
        : comm_manager_(comm_manager), gvt_period_(period), num_worker_threads_(num_worker_threads) {}

    virtual void initialize();
    virtual ~TimeWarpGVTManager() = default;

    void checkProgressGVT();

    virtual bool readyToStart() = 0;

    virtual void progressGVT() = 0;

    virtual void receiveEventUpdate(std::shared_ptr<Event>& event, Color color) = 0;

    virtual Color sendEventUpdate(std::shared_ptr<Event>& event) = 0;

    virtual void reportThreadMin(unsigned int timestamp, unsigned int thread_id,
                                 unsigned int local_gvt_flag) = 0;

    virtual void reportThreadMin(unsigned int timestamp, unsigned int thread_id);

    virtual void reportThreadSendMin(unsigned int timestamp, unsigned int thread_id) = 0;

    virtual unsigned int getLocalGVTFlag() = 0;

    virtual bool getGVTFlag();

    virtual bool gvtUpdated() = 0;

    virtual void workerThreadGVTBarrierSync();

    virtual int64_t getMessageCount() = 0;

    unsigned int getGVT() { return gVT_; }

protected:
    const std::shared_ptr<TimeWarpCommunicationManager> comm_manager_;

    unsigned int gVT_ = 0;

    std::chrono::time_point<std::chrono::steady_clock> gvt_start;

    std::chrono::time_point<std::chrono::steady_clock> gvt_stop;

    unsigned int gvt_period_;

    GVTState gvt_state_;

    unsigned int num_worker_threads_;

};

}

#endif
