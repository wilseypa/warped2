#ifndef MATTERN_GVT_MANAGER_HPP
#define MATTERN_GVT_MANAGER_HPP

#include <memory> // for unique_ptr

#include "TimeWarpEventDispatcher.hpp"
#include "TimeWarpGVTManager.hpp"
#include "serialization.hpp"
#include "TimeWarpKernelMessage.hpp"

/*  This class implements the Mattern GVT algorithm and provides methods
 *  for initiating a calculation cycle and for processing received tokens
 */

namespace warped {

// Color of messages for matterns algorithm
enum class MatternColor { WHITE, RED };

class TimeWarpMatternGVTManager : public TimeWarpGVTManager {
public:
    TimeWarpMatternGVTManager(std::shared_ptr<TimeWarpCommunicationManager> comm_manager,
        unsigned int period) :
        TimeWarpGVTManager(comm_manager, period) {}

    void initialize();

    // Starts the GVT calculation process
    MessageFlags calculateGVT();

    // Called when a MatternGVTToken has been received
    MessageFlags receiveMatternGVTToken(std::unique_ptr<TimeWarpKernelMessage> msg);

    void sendMatternGVTToken(unsigned int local_minimum);

    void setGvtInfo(int color);

    int getGvtInfo(unsigned int timestamp);

    bool checkGVTPeriod();

protected:
    unsigned int infinityVT();

private:
    MatternColor color_ = MatternColor::WHITE;
    unsigned int white_msg_counter_ = 0;
    unsigned int min_red_msg_timestamp_ = infinityVT();

    unsigned int min_of_all_lvt_ = infinityVT();

    bool gVT_token_pending_ = false;

};

struct MatternGVTToken : public TimeWarpKernelMessage {
    MatternGVTToken() = default;
    MatternGVTToken(unsigned int sender, unsigned int receiver, unsigned int mc, unsigned int ms,
                    unsigned int c) :
        TimeWarpKernelMessage(sender, receiver),
        m_clock(mc),
        m_send(ms),
        count(c) {}

    unsigned int m_clock;
    unsigned int m_send;
    unsigned int count;

    MessageType get_type() { return MessageType::MatternGVTToken; }

    WARPED_REGISTER_SERIALIZABLE_MEMBERS(cereal::base_class<TimeWarpKernelMessage>(this), m_clock,
                                         m_send, count)

};

} // warped namespace

#endif
