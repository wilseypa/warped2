#ifndef MATTERN_GVT_MANAGER_HPP
#define MATTERN_GVT_MANAGER_HPP

#include <memory> // for unique_ptr
#include <functional> // for std::function

#include "TimeWarpEventDispatcher.hpp"
#include "GVTManager.hpp"
#include "serialization.hpp"
#include "KernelMessage.hpp"
#include "GVTUpdateMessage.hpp"

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
    MatternGVTManager(std::shared_ptr<CommunicationManager> comm_manager, unsigned int period) :
        GVTManager(comm_manager, period) {}

    // Starts the GVT calculation process
    bool calculateGVT();

    // Called when a MatternGVTToken has been received
    bool receiveMatternGVTToken(std::unique_ptr<MatternGVTToken> msg);

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

} // warped namespace

#endif
