#ifndef TIME_WARP_EVENT_DISPATCHER_H
#define TIME_WARP_EVENT_DISPATCHER_H

#include <atomic>
#include <memory>
#include <unordered_map>
#include <string>
#include <vector>
#include <array>

#include "TimeWarpCommunicationManager.hpp"
#include "EventDispatcher.hpp"
#include "Event.hpp"

namespace warped {

class LTSFQueue;
class Partitioner;
class SimulationObject;
class TimeWarpCommunicationManager;
class TimeWarpGVTManager;
class TimeWarpStateManager;
class TimeWarpOutputManager;

// This is the EventDispatcher that will run a Time Warp synchronized parallel simulation.

class TimeWarpEventDispatcher : public EventDispatcher {
public:
    TimeWarpEventDispatcher(unsigned int max_sim_time,
        unsigned int num_worker_threads,
        std::shared_ptr<TimeWarpCommunicationManager> comm_manager,
        std::unique_ptr<LTSFQueue> events,
        std::unique_ptr<TimeWarpGVTManager> gvt_manager,
        std::unique_ptr<TimeWarpStateManager> state_manager,
        std::unique_ptr<TimeWarpOutputManager> output_manager);

    void startSimulation(const std::vector<std::vector<SimulationObject*>>& objects);

    void initialize(const std::vector<std::vector<SimulationObject*>>& objects);

    MessageFlags receiveEventMessage(std::unique_ptr<TimeWarpKernelMessage> kmsg);

    void sendRemoteEvent(std::unique_ptr<Event> event, unsigned int receiver_id);

    void sendLocalEvent(std::unique_ptr<Event> event, unsigned int thread_id);

    void fossilCollect(unsigned int gvt);

    void cancelEvents(std::unique_ptr<std::vector<std::unique_ptr<Event>>> events_to_cancel);

    void rollback(unsigned int straggler_time, unsigned int local_object_id, SimulationObject* ob);

    unsigned int getMinimumLVT();

private:
    void processEvents();

    unsigned int num_worker_threads_;

    std::unordered_map<std::string, SimulationObject*> objects_by_name_;
    std::unordered_map<std::string, unsigned int> local_object_id_by_name_;
    std::unordered_map<std::string, unsigned int> object_node_id_by_name_;

    const std::shared_ptr<TimeWarpCommunicationManager> comm_manager_;
    const std::unique_ptr<LTSFQueue> events_;
    const std::unique_ptr<TimeWarpGVTManager> gvt_manager_;
    const std::unique_ptr<TimeWarpStateManager> state_manager_;
    const std::unique_ptr<TimeWarpOutputManager> output_manager_;

    // flag to initiate gvt calculation
    std::atomic<bool> calculate_gvt_ = ATOMIC_VAR_INIT(false);

    // flag to initiate minimum lvt calculation
    std::atomic<unsigned int> min_lvt_flag_ = ATOMIC_VAR_INIT(0);

    // local min lvt of each worker thread
    std::unique_ptr<unsigned int []> min_lvt_by_thread_id_;

    // minimum timestamp of sent events used for minimum lvt calculation
    std::unique_ptr<unsigned int []> send_min_by_thread_id_;

    // flag to determine if worker thread has already calculated min lvt
    std::unique_ptr<bool []> calculated_min_lvt_by_thread_id_;
};

enum class MatternColor;

struct EventMessage : public TimeWarpKernelMessage {
    EventMessage() = default;
    EventMessage(unsigned int sender, unsigned int receiver, std::unique_ptr<Event> e,
        int c) :
        TimeWarpKernelMessage(sender, receiver),
        event(std::move(e)),
        gvt_mattern_color(c) {}

    std::unique_ptr<Event> event;
    int gvt_mattern_color; // 0 for white, 1 for red

    MessageType get_type() { return MessageType::EventMessage; }

    WARPED_REGISTER_SERIALIZABLE_MEMBERS(cereal::base_class<TimeWarpKernelMessage>(this), event,
        gvt_mattern_color)
};


} // namespace warped

#endif
