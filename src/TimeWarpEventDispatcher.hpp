#ifndef TIME_WARP_EVENT_DISPATCHER_H
#define TIME_WARP_EVENT_DISPATCHER_H

#ifdef __GNUC__ 
  #if __GNUC_MINOR__ < 8
    #define thread_local __thread
  #endif
#endif

#include <atomic>
#include <memory>
#include <unordered_map>
#include <string>
#include <vector>
#include <deque>
#include <thread>

#include "EventDispatcher.hpp"
#include "Event.hpp"
#include "TimeWarpCommunicationManager.hpp"

namespace warped {

class LTSFQueue;
class Partitioner;
class SimulationObject;
class TimeWarpStateManager;
class TimeWarpOutputManager;
class TimeWarpFileStreamManager;
class TimeWarpEventSet;
class TimeWarpMatternGVTManager;
class TimeWarpLocalGVTManager;
class TimeWarpTerminationManager;
enum class MatternColor;

// This is the EventDispatcher that will run a Time Warp synchronized parallel simulation.

class TimeWarpEventDispatcher : public EventDispatcher {
public:
    TimeWarpEventDispatcher(unsigned int max_sim_time,
        unsigned int num_worker_threads,
        unsigned int num_schedulers,
        std::shared_ptr<TimeWarpCommunicationManager> comm_manager,
        std::unique_ptr<TimeWarpEventSet> event_set,
        std::unique_ptr<TimeWarpMatternGVTManager> mattern_gvt_manager,
        std::unique_ptr<TimeWarpLocalGVTManager> local_gvt_manager,
        std::unique_ptr<TimeWarpStateManager> state_manager,
        std::unique_ptr<TimeWarpOutputManager> output_manager,
        std::unique_ptr<TimeWarpFileStreamManager> twfs_manager,
        std::unique_ptr<TimeWarpTerminationManager> termination_manager);

    void startSimulation(const std::vector<std::vector<SimulationObject*>>& objects);

private:

    void sendLocalEvent(std::shared_ptr<Event> event);

    void sendLocalNegEvent(std::shared_ptr<Event> event, unsigned int obj_id);

    void fossilCollect(unsigned int gvt);

    void cancelEvents(unsigned int sender_local_obj_id, 
                        std::unique_ptr<std::vector<std::shared_ptr<Event>>> events_to_cancel);

    void rollback(std::shared_ptr<Event> straggler_event);

    void coastForward(std::shared_ptr<Event> stop_event, 
                            std::shared_ptr<Event> restored_state_event);

    FileStream& getFileStream(SimulationObject *object, const std::string& filename,
        std::ios_base::openmode mode, std::shared_ptr<Event> this_event);

    void enqueueRemoteEvent(std::shared_ptr<Event> event, unsigned int receiver_id);

    void processEvents(unsigned int id);

/* ====================== Used by only manager thread ========================= */

    void initialize(const std::vector<std::vector<SimulationObject*>>& objects);

    void receiveEventMessage(std::unique_ptr<TimeWarpKernelMessage> kmsg);

    void sendRemoteEvents();

/* ============================================================================ */

    unsigned int num_worker_threads_;

    unsigned int num_schedulers_;

    unsigned int num_local_objects_;

    std::unordered_map<std::string, SimulationObject*> objects_by_name_;
    std::unordered_map<std::string, unsigned int> local_object_id_by_name_;
    std::unordered_map<std::string, unsigned int> object_node_id_by_name_;

    const std::shared_ptr<TimeWarpCommunicationManager> comm_manager_;
    const std::unique_ptr<TimeWarpEventSet> event_set_;
    const std::unique_ptr<TimeWarpMatternGVTManager> mattern_gvt_manager_;
    const std::unique_ptr<TimeWarpLocalGVTManager> local_gvt_manager_;
    const std::unique_ptr<TimeWarpStateManager> state_manager_;
    const std::unique_ptr<TimeWarpOutputManager> output_manager_;
    const std::unique_ptr<TimeWarpFileStreamManager> twfs_manager_;
    const std::unique_ptr<TimeWarpTerminationManager> termination_manager_;

    std::unique_ptr<unsigned int []> object_simulation_time_;

    std::unique_ptr<std::atomic<unsigned long> []> event_counter_by_obj_;

    std::deque<std::tuple <std::shared_ptr<Event>, unsigned int, MatternColor>> remote_event_queue_;
    std::mutex remote_event_queue_lock_;

    static thread_local unsigned int thread_id;

    // Rollback counter
    std::atomic<unsigned long> rollback_count_;
};

struct EventMessage : public TimeWarpKernelMessage {
    EventMessage() = default;
    EventMessage(unsigned int sender, unsigned int receiver, std::shared_ptr<Event> e,
      MatternColor c) :
        TimeWarpKernelMessage(sender, receiver),
        event(e),
        gvt_mattern_color(c) {}

    std::shared_ptr<Event> event;
    MatternColor gvt_mattern_color; // 0 for white, 1 for red

    MessageType get_type() { return MessageType::EventMessage; }

    WARPED_REGISTER_SERIALIZABLE_MEMBERS(cereal::base_class<TimeWarpKernelMessage>(this), event,
        gvt_mattern_color)
};

} // namespace warped

#endif
