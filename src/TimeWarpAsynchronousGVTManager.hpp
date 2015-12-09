#ifndef ASYNCHRONOUS_GVT_MANAGER_HPP
#define ASYNCHRONOUS_GVT_MANAGER_HPP

#include <memory> // for unique_ptr
#include <mutex>

#include "TimeWarpEventDispatcher.hpp"
#include "TimeWarpGVTManager.hpp"
#include "serialization.hpp"
#include "TimeWarpKernelMessage.hpp"

namespace warped {

class TimeWarpAsynchronousGVTManager : public TimeWarpGVTManager {
public:
    TimeWarpAsynchronousGVTManager(std::shared_ptr<TimeWarpCommunicationManager> comm_manager,
        unsigned int period, unsigned int num_worker_threads)
        : TimeWarpGVTManager(comm_manager, period, num_worker_threads) {}

    // Registers message handlers and initializes data
    void initialize() override;

    bool readyToStart();

    void progressGVT();

    void receiveEventUpdate(std::shared_ptr<Event>& event, Color color);

    Color sendEventUpdate(std::shared_ptr<Event>& event);

    bool gvtUpdated();

    int64_t getMessageCount() {
        return initialColorCount();
    }

    void reportThreadMin(unsigned int timestamp, unsigned int thread_id,
                                 unsigned int local_gvt_flag);

    void reportThreadSendMin(unsigned int timestamp, unsigned int thread_id);

    unsigned int getLocalGVTFlag();

protected:
    // Message handler for a Mattern token
    void receiveMatternGVTToken(std::unique_ptr<TimeWarpKernelMessage> msg);

    // Message handler for GVT update message
    void receiveGVTUpdate(std::unique_ptr<TimeWarpKernelMessage> kmsg);

    void sendMatternGVTToken(unsigned int local_min);

    void sendGVTUpdate(unsigned int gvt);

    void toggleColor();

    void toggleInitialColor();

    int& initialColorCount();


    void resetLocalState();

private:
    // State variables that must be protected by a lock
    struct MatternNodeState {
        Color color_ = Color::WHITE;
        Color initial_color_ = Color::WHITE;
        int white_msg_count_ = 0;
        int red_msg_count_ = 0;
        unsigned int min_timestamp_ = (unsigned int)-1;
        std::mutex lock_;
    } state_;

    // Accumulated clock minimum
    unsigned int global_min_clock_;

    // Used to hold accumulated white msg count from last token received
    unsigned int msg_count_ = 0;

    // Flag to indicate that GVT has been updated
    bool gvt_updated_ = false;

    bool started_global_gvt_ = false;


    std::atomic<unsigned int> local_gvt_flag_ = ATOMIC_VAR_INIT(0);

    // local minimum of each object including unprocessed events and sent events
    std::unique_ptr<unsigned int []> local_min_;

    // minimum timestamp of sent events used for minimum local min calculation
    std::unique_ptr<unsigned int []> send_min_;

    // flag to determine if object has already calculated min
    std::unique_ptr<bool []> calculated_min_flag_;

    bool started_local_gvt_ = false;
};

struct MatternGVTToken : public TimeWarpKernelMessage {
    MatternGVTToken() = default;
    MatternGVTToken(unsigned int sender, unsigned int receiver, unsigned int mc, unsigned int ms,
        unsigned int c) :
        TimeWarpKernelMessage(sender, receiver),
        m_clock(mc),
        m_send(ms),
        count(c) {}

    // Accumulated minimum of all local simulation clocks
    unsigned int m_clock;

    // Accumulated minimum of all red message timestamps
    unsigned int m_send;

    // Accumulated total of white messages in transit
    int count;

    MessageType get_type() { return MessageType::MatternGVTToken; }

    WARPED_REGISTER_SERIALIZABLE_MEMBERS(cereal::base_class<TimeWarpKernelMessage>(this), m_clock,
                                         m_send, count)

};

struct GVTUpdateMessage : public TimeWarpKernelMessage {
    GVTUpdateMessage() = default;
    GVTUpdateMessage(unsigned int sender_id, unsigned int receiver_id, unsigned int gvt) :
        TimeWarpKernelMessage(sender_id, receiver_id), new_gvt(gvt) {}

    unsigned int new_gvt;

    MessageType get_type() { return MessageType::GVTUpdateMessage; }

    WARPED_REGISTER_SERIALIZABLE_MEMBERS(cereal::base_class<TimeWarpKernelMessage>(this), new_gvt)
};

} // warped namespace

#endif
