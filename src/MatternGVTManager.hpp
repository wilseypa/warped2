#ifndef MATTERN_GVT_MANAGER_HPP
#define MATTERN_GVT_MANAGER_HPP

#include <memory> // for unique_ptr
#include <functional> // for std::function

#include "TimeWarpEventDispatcher.hpp"
#include "GVTManager.hpp"
#include "serialization.hpp"
#include "KernelMessage.hpp"

/*  This class implements the Mattern GVT algorithm and provides methods
 *  for initiating a calculation cycle and for processing received tokens
 */

namespace warped {

// Color of messages for matterns algorithm
enum class MatternColor { WHITE, RED };

struct MatternGVTToken : public KernelMessage {
    MatternGVTToken() = default;
    MatternGVTToken(unsigned int sender, unsigned int receiver, unsigned int mc, unsigned int ms,
                    unsigned int c) :
        KernelMessage(sender, receiver),
        m_clock(mc),
        m_send(ms),
        count(c) {}

    unsigned int m_clock;
    unsigned int m_send;
    unsigned int count;

    MessageType get_type() { return MessageType::MatternGVTToken; }

    WARPED_REGISTER_SERIALIZABLE_MEMBERS(cereal::base_class<KernelMessage>(this), m_clock,
                                         m_send, count)

};

class MatternGVTManager : public GVTManager {
public:
    MatternGVTManager(TimeWarpEventDispatcher* event_dispatcher) :
        event_dispatcher_(event_dispatcher) {}

    // Starts the GVT calculation process
    void calculateGVT();

    // Called when a MatternGVTToken has been received
    void receiveMatternGVTToken(std::unique_ptr<MatternGVTToken> msg);

    void receiveMessage(std::unique_ptr<KernelMessage> msg);

    // Called when a white message is sent
    void incrementWhiteMsgCount() { white_msg_counter_++; }

    // Called when a white message is received
    void decrementWhiteMsgCount() { white_msg_counter_--; }

    // Returns the color of this node, this is needed when receiving an event.
    MatternColor getColor() { return color_; }

protected:
    unsigned int infinityVT();

    void sendMatternGVTToken(std::unique_ptr<MatternGVTToken> msg);

    void sendGVTUpdate();

private:
    MatternColor color_ = MatternColor::WHITE;
    unsigned int white_msg_counter_ = 0;
    unsigned int min_red_msg_timestamp_ = infinityVT();

    bool gVT_calc_initiated_ = false;

    TimeWarpEventDispatcher* event_dispatcher_;

};

} // warped namespace

#endif
