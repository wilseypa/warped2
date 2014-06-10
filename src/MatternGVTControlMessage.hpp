#ifndef MATTERN_GVT_CONTROL_MESSAGE
#define MATTERN_GVT_CONTROL_MESSAGE

namespace warped {

class MatternGVTControlMessage {
public:
    uint64_t mClock() { return m_clock_; }
    uint64_t mSend() { return m_send_; }
    uint32_t count() { return count_; }

private:
    uint64_t m_clock_;
    uint64_t m_send_;
    uint32_t count_;

    WARPED_REGISTER_SERIALIZABLE_MEMBERS(m_clock, m_send, count)
};
//WARPED_REGISTER_POLYMORPHIC_SERIALIZABLE_CLASS CEREAL_REGISTER_TYPE

} // warped namespace

#endif
