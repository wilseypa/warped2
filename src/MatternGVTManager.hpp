#ifndef MATTERN_GVT_MANAGER_HPP
#define MATTERN_GVT_MANAGER_HPP

#include "GVTManager.hpp"
#include "serialization.hpp"

#include <cstdint> // for uint64_t and uint32_t
#include <memory> // for unique_ptr

namespace warped {

// Color of messages for matterns algorithm
enum class MatternMsgColor { WHITE, RED };

struct MatternGVTControlMessage {

    uint64_t m_clock;
    uint64_t m_send;
    uint32_t count;

    WARPED_REGISTER_SERIALIZABLE_MEMBERS(m_clock, m_send, count)

};

class MatternGVTManager : public GVTManager {
public:
    explicit MatternGVTManager(uint32_t num_partitions, uint32_t node_id)
        : num_partitions_(num_partitions),
          node_id_(node_id) {}

    void sendGVTUpdate();
    void calculateGVT();

    void sendMatternControlMessage(uint64_t m_clock, uint64_t m_send, uint32_t count, uint32_t P);

    void receiveMatternControlMessage(std::unique_ptr<warped::MatternGVTControlMessage> msg);

protected:
    uint64_t infinityVT();

private:
    MatternMsgColor color_ = MatternMsgColor::WHITE;
    uint32_t white_msg_counter_ = 0;
    uint64_t min_red_msg_timestamp_ = 0;

    uint32_t num_partitions_;
    uint32_t node_id_;

    bool gVT_calc_initiated_ = false;
    uint32_t msg_round_ = 0;
};

} // warped namespace

#endif
