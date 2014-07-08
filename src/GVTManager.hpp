#ifndef GVT_MANAGER_HPP
#define GVT_MANAGER_HPP

/* This class provides an interface for a specific GVT Manager that
 * implements a specific algorithm for calculating the GVT
 */

#include <string>
#include <functional>

#include "CommunicationManager.hpp"
#include "GVTUpdateMessage.hpp"

namespace warped {

class GVTManager {
public:
    GVTManager(std::shared_ptr<CommunicationManager> comm_manager, unsigned int period) :
        comm_manager_(comm_manager), gvt_period_(period) {}

    virtual bool calculateGVT() = 0;

    virtual void setGvtInfo(int) = 0;

    virtual int getGvtInfo(unsigned int) = 0;

    virtual bool checkGVTPeriod() = 0;

    void sendGVTUpdate();

    void receiveGVTUpdate(std::unique_ptr<GVTUpdateMessage> msg);

    unsigned int getGVT() { return gVT_; }

    void setGVT(uint64_t g) { gVT_ = g; }

protected:

    unsigned int gVT_;

    const std::shared_ptr<CommunicationManager> comm_manager_;

    unsigned int gvt_period_counter_ = 0;
    unsigned int gvt_period_;

};

} // warped namespace

#endif
