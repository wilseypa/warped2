#ifndef MATTERN_GVT_CONTROL_MESSAGE
#define MATTERN_GVT_CONTROL_MESSAGE

#include <cstdint> // for uint32_t and uint64_t

#include "serialization.hpp"

namespace warped {

class MatternGVTControlMessage {
public:
    MatternGVTControlMessage() = default;
    MatternGVTControlMessage(uint64_t m_clock, uint64_t m_send, uint32_t count)
        : m_clock_(m_clock),
          m_send_(m_send),
          count_(count) {}

    uint64_t mClock() { return m_clock_; }
    uint64_t mSend() { return m_send_; }
    uint32_t count() { return count_; }

    WARPED_REGISTER_SERIALIZABLE_MEMBERS(m_clock_, m_send_, count_)

private:
    uint64_t m_clock_;
    uint64_t m_send_;
    uint32_t count_;
};

} // warped namespace

#endif
