#ifndef MATTERN_GVT_MANAGER_HPP
#define MATTERN_GVT_MANAGER_HPP

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
    MatternGVTToken() = default;
    MatternGVTToken(unsigned int mc, unsigned int ms, unsigned int c) :
        m_clock(mc),
        m_send(ms),
        count(c) {}

    unsigned int m_clock;
    unsigned int m_send;
    unsigned int count;

    WARPED_REGISTER_SERIALIZABLE_MEMBERS(m_clock, m_send, count)

};

class MatternGVTManager : public GVTManager {
public:
    MatternGVTManager(unsigned int num_nodes, unsigned int node_id)
        : num_nodes_(num_nodes),
          node_id_(node_id) {}

    // Starts the GVT calculation process
    void calculateGVT();

    // Called when a MatternGVTToken has been received
    void receiveMatternGVTToken(std::unique_ptr<warped::MatternGVTToken> msg);

    // Called when a white message is sent
    void incrementWhiteMsgCount() { white_msg_counter_++; }

    // Called when a white message is received
    void decrementWhiteMsgCount() { white_msg_counter_--; }

protected:
    unsigned int infinityVT();

    void sendMatternGVTToken(std::unique_ptr<MatternGVTToken> msg, unsigned int P);

    void sendGVTUpdate();

private:
    MatternMsgColor color_ = MatternMsgColor::WHITE;
    unsigned int white_msg_counter_ = 0;
    unsigned int min_red_msg_timestamp_ = infinityVT();

    unsigned int num_nodes_;
    unsigned int  node_id_;

    bool gVT_calc_initiated_ = false;

};

} // warped namespace

#endif
