#ifndef MATTERN_GVT_MANAGER_HPP
#define MATTERN_GVT_MANAGER_HPP

#include <cstdint> // for uint64_t and uint32_t
#include <memory> // for unique_ptr

#include "GVTManager.hpp"
#include "serialization.hpp"

/*  This class implements the Mattern GVT algorithm and provides methods
 *  for initiating a calculation cycle and for processing received tokens
 */

namespace warped {

// Color of messages for matterns algorithm
enum class MatternMsgColor { WHITE, RED };

struct MatternGVTToken {
    MatternGVTToken(uint64_t mc, uint64_t ms, uint32_t c) :
        m_clock(mc),
        m_send(ms),
        count(c) {}

    uint64_t m_clock;
    uint64_t m_send;
    uint32_t count;

    WARPED_REGISTER_SERIALIZABLE_MEMBERS(m_clock, m_send, count)

};

class MatternGVTManager : public GVTManager {
public:
    MatternGVTManager(uint32_t num_nodes, uint32_t node_id)
        : num_nodes_(num_nodes),
          node_id_(node_id) {}

    // Starts the GVT calculation process by passing out a MatternGVTToken
    void calculateGVT();

    // Called when a MatternGVTToken has been received
    void receiveMatternGVTToken(std::unique_ptr<warped::MatternGVTToken> msg);

    // Called when a white message is sent
    void incrementWhiteMsgCount() { white_msg_counter_++; }

    // Called when a white message is received
    void decrementWhiteMsgCount() { white_msg_counter_--; }

protected:
    uint64_t infinityVT();

    void sendMatternGVTToken(std::unique_ptr<MatternGVTToken> msg, uint32_t P);

    void sendGVTUpdate();

private:
    MatternMsgColor color_ = MatternMsgColor::WHITE;
    uint32_t white_msg_counter_ = 0;
    uint64_t min_red_msg_timestamp_ = infinityVT();

    uint32_t num_nodes_;
    uint32_t node_id_;

    bool gVT_calc_initiated_ = false;
};

} // warped namespace

#endif
