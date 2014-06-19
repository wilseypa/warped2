#ifndef GVT_MANAGER_HPP
#define GVT_MANAGER_HPP

/* This class provides an interface for a specific GVT Manager that
 * implements a specific algorithm for calculating the GVT
 */

#include <functional>
#include "MPICommunicationManager.hpp"

namespace warped {

class GVTManager {
public:

    virtual void sendGVTUpdate() = 0;

    virtual void calculateGVT(std::function<unsigned int()> f, MPICommunicationManager *mcm) = 0;

    virtual void receiveMessage(std::unique_ptr<KernelMessage> msg, std::function<unsigned int()> f,
        MPICommunicationManager *mpi_manager) = 0;

    unsigned int getGVT() { return gVT_; }

    void setGVT(uint64_t g) { gVT_ = g; }

protected:

    unsigned int gVT_;

};

} // warped namespace

#endif
