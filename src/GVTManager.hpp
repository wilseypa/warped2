#ifndef GVT_MANAGER_HPP
#define GVT_MANAGER_HPP

/* This class provides an interface for a specific GVT Manager that
 * implements a specific algorithm for calculating the GVT
 */

namespace {

class GVTManager {
public:

    virtual void sendGVTUpdate() = 0;

    virtual void calculateGVT() = 0;

    unsigned int getGVT() { return gVT_; }

    void setGVT(uint64_t g) { gVT_ = g; }

protected:

    unsigned int gVT_;

};

} // warped namespace

#endif
