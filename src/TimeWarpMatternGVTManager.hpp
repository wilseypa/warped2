#ifndef MATTERN_GVT_MANAGER_HPP
#define MATTERN_GVT_MANAGER_HPP

#include <memory> // for unique_ptr

#include "TimeWarpEventDispatcher.hpp"
#include "TimeWarpGVTManager.hpp"
#include "serialization.hpp"
#include "TimeWarpKernelMessage.hpp"

/* This class implements the Mattern algorithm, which is a distributed token passing algorithm used
 * to estimate the GVT. Each node contains a single object of this class.
 *
 * Each node is either red or white, all initially white.
 * Event messages which contain timestamps are colored when they are sent. White nodes will send
 * white messages and red nodes will send red messages.
 *
 * Each node must keep track of:
 *  1. It's color
 *  2. Accumulated minumum timestamp of red messages it has received
 *      (after red message has been received)
 *  3. White messages it has sent minus white messages it has received (NOTE: This is one version
 *      of the algorithm. Vector counters could be used instead to keep track of any outstanding
 *      white messages at all nodes.)
 *
 * A mattern token consists of:
 *  1. Accumulated value of the minimum of all local simulation times (lvt values)
 *  2. Accumulated minimum of the red message timestamps for all nodes that have received token
 *  3. Accumulated count of all white messages in transit (This could also be a vector count
 *      consisting of outstanding messages at all nodes)
 *
 * Description of implemented algorithm:
 *  1. The designated node (mpi_rank == 0) starts the gvt estimation by sending an initial token
 *       with its min lvt, min red message timestamp of infinity and its white msg counter.
 *  2. When another node receives a token, if it is already red then the min red msg timestamp
 *      is updated. If it is white, then it becomes red.
 *  3. When the initiator node (mpi_rank = 0) receives a the toke back and the accumulated white
 *      msg count is zero, then the gvt is estimated by taking the minimum of the min red msg
 *      timestamps and the minimum of all local simulation times. If the accumulated white message
 *      count is not zero, another round of tokens is inititated.
 *  NOTE: In this implemenation the token is passed in logical ring fashion using the mpi rank as
 *      an id.
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
    MatternGVTToken(unsigned int receiver, unsigned int mc, unsigned int ms, unsigned int c) :
        TimeWarpKernelMessage(receiver),
        m_clock(mc),
        m_send(ms),
        count(c) {}

    // Accumulated minimum of all local simulation clocks (lvt values)
    unsigned int m_clock;

    // Accumulated minimum of all red message timestamps
    unsigned int m_send;

    // Accumulated total of white messages in transit
    unsigned int count;

    MessageType get_type() { return MessageType::MatternGVTToken; }

    WARPED_REGISTER_SERIALIZABLE_MEMBERS(cereal::base_class<TimeWarpKernelMessage>(this), m_clock,
                                         m_send, count)

};

} // warped namespace

#endif
