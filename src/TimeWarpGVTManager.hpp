#ifndef GVT_MANAGER_HPP
#define GVT_MANAGER_HPP

/* This class provides an interface for a specific GVT Manager that
 * implements a specific algorithm for calculating the GVT
 */

#include <string>
#include <functional>

#include "TimeWarpCommunicationManager.hpp"

namespace warped {

class TimeWarpGVTManager {
public:
    TimeWarpGVTManager(std::shared_ptr<TimeWarpCommunicationManager> comm_manager,
        unsigned int period) :
        comm_manager_(comm_manager), gvt_period_(period) {}

    virtual void initialize() = 0;

    virtual MessageFlags calculateGVT() = 0;

    virtual void setGvtInfo(int) = 0;

    virtual int getGvtInfo(unsigned int) = 0;

    virtual bool checkGVTPeriod() = 0;

    void sendGVTUpdate();

    MessageFlags receiveGVTUpdate(std::unique_ptr<TimeWarpKernelMessage> kmsg);

    unsigned int getGVT() { return gVT_; }

    void setGVT(uint64_t g) { gVT_ = g; }

protected:

    unsigned int gVT_;

    const std::shared_ptr<TimeWarpCommunicationManager> comm_manager_;

    unsigned int gvt_period_counter_ = 0;
    unsigned int gvt_period_;

};

struct GVTUpdateMessage : public TimeWarpKernelMessage {
    GVTUpdateMessage() = default;
    GVTUpdateMessage(unsigned int sender_id, unsigned int receiver_id, unsigned int gvt) :
        TimeWarpKernelMessage(sender_id, receiver_id), new_gvt(gvt) {}

    unsigned int new_gvt;

    MessageType get_type() { return MessageType::GVTUpdateMessage; }

    WARPED_REGISTER_SERIALIZABLE_MEMBERS(cereal::base_class<TimeWarpKernelMessage>(this), new_gvt)
};

} // warped namespace

#endif
