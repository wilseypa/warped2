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

    uint64_t getGVT() { return gVT; }

    void setGVT(uint64_t g) { gVT = g; }

private:

    uint64_t gVT;

};

} // warped namespace

#endif
