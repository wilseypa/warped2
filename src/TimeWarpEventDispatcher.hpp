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
#include "TimeWarpStatistics.hpp"
#include "CircularList.hpp"

namespace warped {

class LTSFQueue;
class Partitioner;
class LogicalProcess;
class TimeWarpStateManager;
class TimeWarpOutputManager;
class TimeWarpFileStreamManager;
class TimeWarpEventSet;
class TimeWarpGVTManager;
class TimeWarpTerminationManager;
enum class Color;

// This is the EventDispatcher that will run a Time Warp synchronized parallel simulation.

class TimeWarpEventDispatcher : public EventDispatcher {
public:
    TimeWarpEventDispatcher(unsigned int max_sim_time,
        unsigned int num_worker_threads,
        bool is_lp_migration_on,
        std::shared_ptr<TimeWarpCommunicationManager> comm_manager,
        std::unique_ptr<TimeWarpEventSet> event_set,
        std::unique_ptr<TimeWarpGVTManager> gvt_manager,
        std::unique_ptr<TimeWarpStateManager> state_manager,
        std::unique_ptr<TimeWarpOutputManager> output_manager,
        std::unique_ptr<TimeWarpFileStreamManager> twfs_manager,
        std::unique_ptr<TimeWarpTerminationManager> termination_manager,
        std::unique_ptr<TimeWarpStatistics> tw_stats);

    void startSimulation(const std::vector<std::vector<LogicalProcess*>>& lps);

private:
    void sendEvents(std::shared_ptr<Event> source_event, std::vector<std::shared_ptr<Event>> new_events,
                    unsigned int sender_lp_id, LogicalProcess *sender_lp);

    void sendLocalEvent(std::shared_ptr<Event> event);

    void cancelEvents(std::unique_ptr<std::vector<std::shared_ptr<Event>>> events_to_cancel);

    void rollback(std::shared_ptr<Event> straggler_event);

    void coastForward(std::shared_ptr<Event> stop_event, 
                            std::shared_ptr<Event> restored_state_event);

    FileStream& getFileStream(LogicalProcess *lp, const std::string& filename,
        std::ios_base::openmode mode, std::shared_ptr<Event> this_event);

    void enqueueRemoteEvent(std::shared_ptr<Event> event, unsigned int receiver_id);

    void processEvents(unsigned int id);

#ifdef TIMEWARP_EVENT_LOG
    std::string eventLogFileName(unsigned int thread_id) {
        return "eventlog_worker_" + std::to_string(thread_id) + ".csv";
    }
#endif

/* ====================== Used by only manager thread ========================= */

    void initialize(const std::vector<std::vector<LogicalProcess*>>& lps);

    void receiveEventMessage(std::unique_ptr<TimeWarpKernelMessage> kmsg);

    void onGVT(unsigned int gvt);

/* ============================================================================ */

    unsigned int num_worker_threads_;
    bool is_lp_migration_on_;
    unsigned int num_local_lps_;

    std::unordered_map<std::string, LogicalProcess*> lps_by_name_;
    std::unordered_map<std::string, unsigned int> local_lp_id_by_name_;

#ifdef TIMEWARP_EVENT_LOG
    // Event log for each worker thread
    std::vector<std::unique_ptr<CircularList<std::string>>> event_log_;
#endif

    const std::shared_ptr<TimeWarpCommunicationManager> comm_manager_;
    const std::unique_ptr<TimeWarpEventSet> event_set_;
    const std::unique_ptr<TimeWarpGVTManager> gvt_manager_;
    const std::unique_ptr<TimeWarpStateManager> state_manager_;
    const std::unique_ptr<TimeWarpOutputManager> output_manager_;
    const std::unique_ptr<TimeWarpFileStreamManager> twfs_manager_;
    const std::unique_ptr<TimeWarpTerminationManager> termination_manager_;
    const std::unique_ptr<TimeWarpStatistics> tw_stats_;

    static thread_local unsigned int thread_id;
};

struct EventMessage : public TimeWarpKernelMessage {
    EventMessage() = default;
    EventMessage(unsigned int sender, unsigned int receiver, std::shared_ptr<Event> e,
      Color c) :
        TimeWarpKernelMessage(sender, receiver),
        event(e),
        color_(c) {}

    std::shared_ptr<Event> event;
    Color color_;

    MessageType get_type() { return MessageType::EventMessage; }

    WARPED_REGISTER_SERIALIZABLE_MEMBERS(cereal::base_class<TimeWarpKernelMessage>(this), event,
        color_)
};

} // namespace warped

#endif
