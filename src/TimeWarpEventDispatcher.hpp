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
#include "TimeWarpFileStream.hpp"
#include "TimeWarpCommunicationManager.hpp"

namespace warped {

class LTSFQueue;
class Partitioner;
class SimulationObject;
class TimeWarpCommunicationManager;
class TimeWarpGVTManager;
class TimeWarpStateManager;
class TimeWarpOutputManager;
class TimeWarpFileStreamManager;
class TimeWarpEventSet;

// This is the EventDispatcher that will run a Time Warp synchronized parallel simulation.

class TimeWarpEventDispatcher : public EventDispatcher {
public:
    TimeWarpEventDispatcher(unsigned int max_sim_time,
        unsigned int num_worker_threads,
        unsigned int num_schedulers,
        std::shared_ptr<TimeWarpCommunicationManager> comm_manager,
        std::unique_ptr<TimeWarpEventSet> event_set,
        std::unique_ptr<TimeWarpGVTManager> gvt_manager,
        std::unique_ptr<TimeWarpStateManager> state_manager,
        std::unique_ptr<TimeWarpOutputManager> output_manager,
        std::unique_ptr<TimeWarpFileStreamManager> twfs_manager);

    void startSimulation(const std::vector<std::vector<SimulationObject*>>& objects);

private:
    void initialize(const std::vector<std::vector<SimulationObject*>>& objects);

    void insertIntoEventSet(unsigned int thread_index, 
            unsigned int object_id, std::shared_ptr<Event> event);

    void sendLocalEvent(std::shared_ptr<Event> event);

    void fossilCollect(unsigned int gvt);

    void cancelEvents(std::unique_ptr<std::vector<std::shared_ptr<Event>>> events_to_cancel);

    void rollback(std::shared_ptr<Event> straggler_event, unsigned int local_object_id, 
                                                                    SimulationObject* ob);

    void coastForward(SimulationObject *object, std::shared_ptr<Event> stop_event,
        std::shared_ptr<Event> restored_state_event);

    FileStream& getFileStream(SimulationObject *object, const std::string& filename,
        std::ios_base::openmode mode, std::shared_ptr<Event> this_event);

    void enqueueRemoteEvent(std::shared_ptr<Event> event, unsigned int receiver_id);

    void processEvents(unsigned int id);

    void populateThreadMap();

/* ====================== Used by only manager thread ========================= */

    MessageFlags receiveEventMessage(std::unique_ptr<TimeWarpKernelMessage> kmsg);

    unsigned int getMinimumLVT();

    void sendRemoteEvents();

    MessageFlags handleReceivedMessages();

    void checkLocalGVTStart(MessageFlags &msg_flags, bool &started_min_lvt);

    void checkGVTUpdate(MessageFlags &msg_flags);

    void checkGlobalGVTStart(bool &calculate_gvt, MessageFlags &msg_flags,
        bool &started_min_lvt, std::chrono::time_point<std::chrono::steady_clock> &gvt_start);

    void checkLocalGVTComplete(MessageFlags &msg_flags, bool &started_min_lvt);

/* ============================================================================ */

    unsigned int num_worker_threads_;

    unsigned int num_schedulers_;

    std::unordered_map<std::string, SimulationObject*> objects_by_name_;
    std::unordered_map<std::string, unsigned int> local_object_id_by_name_;
    std::unordered_map<std::string, unsigned int> object_node_id_by_name_;

    const std::shared_ptr<TimeWarpCommunicationManager> comm_manager_;
    const std::unique_ptr<TimeWarpEventSet> event_set_;
    const std::unique_ptr<TimeWarpGVTManager> gvt_manager_;
    const std::unique_ptr<TimeWarpStateManager> state_manager_;
    const std::unique_ptr<TimeWarpOutputManager> output_manager_;
    const std::unique_ptr<TimeWarpFileStreamManager> twfs_manager_;

    std::unique_ptr<unsigned int []> object_simulation_time_;

    std::unique_ptr<std::atomic<unsigned long> []> event_counter_by_obj_;

    std::unique_ptr<std::shared_ptr<Event> []> straggler_event_list_;
    std::unique_ptr<std::atomic<unsigned int> []> straggler_event_list_lock_;

    std::deque<std::pair <std::shared_ptr<Event>, unsigned int>> remote_event_queue_;
    std::mutex remote_event_queue_lock_;

    static thread_local unsigned int thread_id;

    // flag to initiate minimum lvt calculation
    std::atomic<unsigned int> min_lvt_flag_ = ATOMIC_VAR_INIT(0);

    // local min lvt of each worker thread
    std::unique_ptr<unsigned int []> min_lvt_;

    // minimum timestamp of sent events used for minimum lvt calculation
    std::unique_ptr<unsigned int []> send_min_;

    // flag to determine if worker thread has already calculated min lvt
    std::unique_ptr<bool []> calculated_min_flag_;
};

enum class MatternColor;

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
