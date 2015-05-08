#include "TimeWarpEventDispatcher.hpp"

#include <atomic>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include <cmath>
#include <limits>       // for std::numeric_limits<>::max();
#include <algorithm>    // for std::min
#include <chrono>       // for std::chrono::steady_clock
#include <cstring>      // for std::memset
#include <iostream>
#include <cassert>

#include "Event.hpp"
#include "EventDispatcher.hpp"
#include "LTSFQueue.hpp"
#include "Partitioner.hpp"
#include "SimulationObject.hpp"
#include "TimeWarpMPICommunicationManager.hpp"
#include "TimeWarpMatternGVTManager.hpp"
#include "TimeWarpLocalGVTManager.hpp"
#include "TimeWarpStateManager.hpp"
#include "TimeWarpOutputManager.hpp"
#include "TimeWarpFileStreamManager.hpp"
#include "TimeWarpTerminationManager.hpp"
#include "TimeWarpEventSet.hpp"
#include "utility/memory.hpp"
#include "utility/warnings.hpp"

WARPED_REGISTER_POLYMORPHIC_SERIALIZABLE_CLASS(warped::EventMessage)
WARPED_REGISTER_POLYMORPHIC_SERIALIZABLE_CLASS(warped::Event)
WARPED_REGISTER_POLYMORPHIC_SERIALIZABLE_CLASS(warped::NegativeEvent)

namespace warped {

thread_local unsigned int TimeWarpEventDispatcher::thread_id;

TimeWarpEventDispatcher::TimeWarpEventDispatcher(unsigned int max_sim_time,
    unsigned int num_worker_threads,
    unsigned int num_schedulers,
    std::shared_ptr<TimeWarpCommunicationManager> comm_manager,
    std::unique_ptr<TimeWarpEventSet> event_set,
    std::unique_ptr<TimeWarpMatternGVTManager> mattern_gvt_manager,
    std::unique_ptr<TimeWarpLocalGVTManager> local_gvt_manager,
    std::unique_ptr<TimeWarpStateManager> state_manager,
    std::unique_ptr<TimeWarpOutputManager> output_manager,
    std::unique_ptr<TimeWarpFileStreamManager> twfs_manager,
    std::unique_ptr<TimeWarpTerminationManager> termination_manager,
    std::unique_ptr<TimeWarpStatistics> tw_stats) :
        EventDispatcher(max_sim_time), num_worker_threads_(num_worker_threads),
        num_schedulers_(num_schedulers), comm_manager_(comm_manager),
        event_set_(std::move(event_set)), mattern_gvt_manager_(std::move(mattern_gvt_manager)),
        local_gvt_manager_(std::move(local_gvt_manager)), state_manager_(std::move(state_manager)),
        output_manager_(std::move(output_manager)), twfs_manager_(std::move(twfs_manager)),
        termination_manager_(std::move(termination_manager)), tw_stats_(std::move(tw_stats)) {}

void TimeWarpEventDispatcher::startSimulation(const std::vector<std::vector<SimulationObject*>>&
                                              objects) {
    initialize(objects);

    // Create worker threads
    std::vector<std::thread> threads;
    for (unsigned int i = 0; i < num_worker_threads_; ++i) {
        auto thread(std::thread {&TimeWarpEventDispatcher::processEvents, this, i});
        thread.detach();
        threads.push_back(std::move(thread));
    }

    unsigned int gvt = 0;
    auto sim_start = std::chrono::steady_clock::now();

    // Master thread main loop
    while (!termination_manager_->terminationStatus()) {

        comm_manager_->dispatchReceivedMessages();
        sendRemoteEvents();

        if ((mattern_gvt_manager_->startGVT() || mattern_gvt_manager_->needLocalGVT())
                && !fossil_collect_) {
            local_gvt_manager_->startGVT();
        }

        if (local_gvt_manager_->completeGVT()) {
            if (comm_manager_->getNumProcesses() > 1) {
                mattern_gvt_manager_->sendMatternGVTToken(local_gvt_manager_->getGVT());
            } else {
                gvt = local_gvt_manager_->getGVT();
                std::cout << "GVT: " << gvt << std::endl;
                fossil_collect_ = true;
                tw_stats_->upCount(GVT_CYCLES, num_worker_threads_);
            }
        }

        if (mattern_gvt_manager_->gvtUpdated()) {
            gvt = mattern_gvt_manager_->getGVT();
            if (comm_manager_->getID() == 0) {
                std::cout << "GVT: " << gvt << std::endl;
            }
            fossil_collect_ = true;
            tw_stats_->upCount(GVT_CYCLES, num_worker_threads_);
            mattern_gvt_manager_->resetState();
        }

        comm_manager_->dispatchReceivedMessages();
        sendRemoteEvents();

        fossilCollect(gvt);

        comm_manager_->dispatchReceivedMessages();
        sendRemoteEvents();

        if ((comm_manager_->getID() == 0) && termination_manager_->nodePassive()) {
            if (termination_manager_->sendTerminationToken(State::PASSIVE)) {
                tw_stats_->upCount(TERMINATION_CYCLES, num_worker_threads_);
            }
        }
    }

    comm_manager_->waitForAllProcesses();
    auto sim_stop = std::chrono::steady_clock::now();

    double num_seconds = double((sim_stop - sim_start).count()) *
                std::chrono::steady_clock::period::num / std::chrono::steady_clock::period::den;

    tw_stats_->calculateStats();

    if (comm_manager_->getID() == 0) {
        std::cout << "\nSimulation completed in " << num_seconds << " second(s)" << "\n\n";
        tw_stats_->printStats();
    }
}

void TimeWarpEventDispatcher::processEvents(unsigned int id) {

    thread_id = id;
    unsigned int local_gvt_flag;

    while (!termination_manager_->terminationStatus()) {
        // local_gvt_flag must be obtained before getting the next event
        local_gvt_flag = local_gvt_manager_->getLocalGVTFlag();

        std::shared_ptr<Event> event = event_set_->getEvent(thread_id);
        if (event != nullptr) {

            if (termination_manager_->threadPassive(thread_id)) {
                termination_manager_->setThreadActive(thread_id);
            }

            assert(object_node_id_by_name_[event->receiverName()] == comm_manager_->getID());
            unsigned int current_object_id = local_object_id_by_name_[event->receiverName()];
            SimulationObject* current_object = objects_by_name_[event->receiverName()];

            // Check to see if object needs a rollback
            event_set_->acquireInputQueueLock(current_object_id);
            auto last_processed_event = event_set_->lastProcessedEvent(current_object_id);
            event_set_->releaseInputQueueLock(current_object_id);
            if (last_processed_event && 
                    ((*event < *last_processed_event) || 
                        ((*event == *last_processed_event) && 
                         (event->event_type_ == EventType::NEGATIVE)))) {
                rollback(event);
            }

            // Check to see if event is NEGATIVE
            if (event->event_type_ == EventType::NEGATIVE) {
                event_set_->acquireInputQueueLock(current_object_id);
                event_set_->cancelEvent(current_object_id, event);
                event_set_->startScheduling(current_object_id);
                event_set_->releaseInputQueueLock(current_object_id);

                tw_stats_->upCount(CANCELLED_EVENTS, thread_id);
                continue;
            }

            local_gvt_manager_->receiveEventUpdateState(
                    event->timestamp(), thread_id, local_gvt_flag);

            // Update simulation time
            object_simulation_time_[current_object_id] = event->timestamp();

            // process event and get new events
            auto new_events = current_object->receiveEvent(*event);

            // Save state
            state_manager_->saveState(event, current_object_id, current_object);

            tw_stats_->upCount(EVENTS_PROCESSED, thread_id);

            // Send new events
            sendEvents(new_events, current_object_id, current_object);

            // Move the next event from object into the schedule queue
            // Also transfer old event to processed queue
            event_set_->acquireInputQueueLock(current_object_id);
            event_set_->replenishScheduler(current_object_id);
            event_set_->releaseInputQueueLock(current_object_id);

        } else {
            if (!termination_manager_->threadPassive(thread_id)) {
                termination_manager_->setThreadPassive(thread_id);
            }
            local_gvt_manager_->receiveEventUpdateState((unsigned int)-1,
                thread_id, local_gvt_flag);
        }
    }
}

void TimeWarpEventDispatcher::receiveEventMessage(std::unique_ptr<TimeWarpKernelMessage> kmsg) {

    auto msg = unique_cast<TimeWarpKernelMessage, EventMessage>(std::move(kmsg));
    mattern_gvt_manager_->receiveEventUpdateState(msg->gvt_mattern_color);

    assert(msg->event != nullptr);

    sendLocalEvent(msg->event);
}

void TimeWarpEventDispatcher::sendEvents(std::vector<std::shared_ptr<Event>> new_events,
    unsigned int sender_object_id, SimulationObject *sender_object) {

    for (auto& e: new_events) {

        if (e->timestamp() <= max_sim_time_) {
            e->sender_name_ = sender_object->name_;
            e->counter_ = event_counter_by_obj_[sender_object_id]++;
            e->send_time_ = object_simulation_time_[sender_object_id];

            output_manager_->insertEvent(e, sender_object_id);

            assert(e->timestamp() >= object_simulation_time_[sender_object_id]);

            unsigned int node_id = object_node_id_by_name_[e->receiverName()];
            if (node_id == comm_manager_->getID()) {
                // Local event
                sendLocalEvent(e);
                tw_stats_->upCount(LOCAL_POSITIVE_EVENTS_SENT, thread_id);
            } else {
                // Remote event
                enqueueRemoteEvent(e, node_id);
                tw_stats_->upCount(REMOTE_POSITIVE_EVENTS_SENT, thread_id);
            }

            local_gvt_manager_->sendEventUpdateState(e->timestamp(), thread_id);
        }
    }
}

void TimeWarpEventDispatcher::sendLocalEvent(std::shared_ptr<Event> event) {
    unsigned int receiver_object_id = local_object_id_by_name_[event->receiverName()];

    event_set_->acquireInputQueueLock(receiver_object_id);
    event_set_->insertEvent(receiver_object_id, event);
    event_set_->releaseInputQueueLock(receiver_object_id);

    tw_stats_->upCount(TOTAL_EVENTS_RECEIVED, thread_id);
}

void TimeWarpEventDispatcher::fossilCollect(unsigned int gvt) {
    unsigned int event_fossil_collect_time;

    if (!fossil_collect_) {
        return;
    }

    for (unsigned int i = 0; i < 100; i++) {

        if (curr_fc_object_id_ >= num_local_objects_) {
            curr_fc_object_id_ = 0;
            fossil_collect_ = false;
            break;
        }

        twfs_manager_->fossilCollect(gvt, curr_fc_object_id_);
        output_manager_->fossilCollect(gvt, curr_fc_object_id_);
        event_fossil_collect_time = state_manager_->fossilCollect(gvt, curr_fc_object_id_);

        event_set_->acquireInputQueueLock(curr_fc_object_id_);
        event_set_->fossilCollect(event_fossil_collect_time, curr_fc_object_id_);
        event_set_->releaseInputQueueLock(curr_fc_object_id_);

        curr_fc_object_id_++;
    }
}

void TimeWarpEventDispatcher::cancelEvents(
            std::unique_ptr<std::vector<std::shared_ptr<Event>>> events_to_cancel) {

    if (events_to_cancel->empty()) return;

    do {
        auto event = events_to_cancel->back();
        auto neg_event = std::make_shared<NegativeEvent>(event);
        events_to_cancel->pop_back();

        if (event->timestamp() <= max_sim_time_) {
            unsigned int receiver_node_id = object_node_id_by_name_[event->receiverName()];
            if (receiver_node_id == comm_manager_->getID()) {
                sendLocalEvent(neg_event);
                tw_stats_->upCount(LOCAL_NEGATIVE_EVENTS_SENT, thread_id);
            } else {
                enqueueRemoteEvent(neg_event, receiver_node_id);
                tw_stats_->upCount(REMOTE_NEGATIVE_EVENTS_SENT, thread_id);
            }
        }

    } while (!events_to_cancel->empty());
}

void TimeWarpEventDispatcher::rollback(std::shared_ptr<Event> straggler_event) {

    unsigned int local_object_id = local_object_id_by_name_[straggler_event->receiverName()];
    SimulationObject* current_object = objects_by_name_[straggler_event->receiverName()];

    twfs_manager_->rollback(straggler_event, local_object_id);

    assert(straggler_event->timestamp() >= mattern_gvt_manager_->getGVT());

    event_set_->acquireInputQueueLock(local_object_id);
    event_set_->rollback(local_object_id, straggler_event);
    event_set_->releaseInputQueueLock(local_object_id);

    auto events_to_cancel = output_manager_->rollback(straggler_event, local_object_id);
    if (events_to_cancel != nullptr) {
        cancelEvents(std::move(events_to_cancel));
    }

    auto restored_state_event = state_manager_->restoreState(
                                            straggler_event, local_object_id, current_object);
    assert(restored_state_event);
    assert(*restored_state_event < *straggler_event);
    object_simulation_time_[local_object_id] = restored_state_event->timestamp();

    if (straggler_event->event_type_ == EventType::POSITIVE) {
        tw_stats_->upCount(PRIMARY_ROLLBACKS, thread_id);
    } else {
        tw_stats_->upCount(SECONDARY_ROLLBACKS, thread_id);
    }

    coastForward(straggler_event, restored_state_event);
}

void TimeWarpEventDispatcher::coastForward(std::shared_ptr<Event> straggler_event, 
                                            std::shared_ptr<Event> restored_state_event) {

    SimulationObject* object = objects_by_name_[straggler_event->receiverName()];
    unsigned int current_object_id = local_object_id_by_name_[straggler_event->receiverName()];

    event_set_->acquireInputQueueLock(current_object_id);
    auto events = event_set_->getEventsForCoastForward(
                                    current_object_id, straggler_event, restored_state_event);
    event_set_->releaseInputQueueLock(current_object_id);
    for (auto event_riterator = events->rbegin();
                    event_riterator != events->rend(); event_riterator++) {
        assert((*event_riterator)->timestamp() <= straggler_event->timestamp());
        assert((*event_riterator)->timestamp() >= object_simulation_time_[current_object_id]);
        object_simulation_time_[current_object_id] = (*event_riterator)->timestamp();
        object->receiveEvent(**event_riterator);
        state_manager_->saveState(*event_riterator, current_object_id, object);
    }
}

void TimeWarpEventDispatcher::initialize(
        const std::vector<std::vector<SimulationObject*>>& objects) {

    num_local_objects_ = objects[comm_manager_->getID()].size();
    event_set_->initialize(num_local_objects_, num_schedulers_, num_worker_threads_);

    object_simulation_time_ = make_unique<unsigned int []>(num_local_objects_);
    std::memset(object_simulation_time_.get(), 0, num_local_objects_*sizeof(unsigned int));

    event_counter_by_obj_ = make_unique<std::atomic<unsigned long> []>(num_local_objects_);

    unsigned int partition_id = 0;
    for (auto& partition : objects) {
        unsigned int object_id = 0;
        for (auto& ob : partition) {
            if (partition_id == comm_manager_->getID()) {
                objects_by_name_[ob->name_] = ob;
                local_object_id_by_name_[ob->name_] = object_id;
                event_counter_by_obj_[object_id].store(0);
                object_id++;
            }
            object_node_id_by_name_[ob->name_] = partition_id;
        }
        partition_id++;
    }

    // Creates the state queues, output queues, and filestream queues for each local object
    state_manager_->initialize(num_local_objects_);
    output_manager_->initialize(num_local_objects_);
    twfs_manager_->initialize(num_local_objects_);

    // Register message handlers
    mattern_gvt_manager_->initialize();
    WARPED_REGISTER_MSG_HANDLER(TimeWarpEventDispatcher, receiveEventMessage, EventMessage);

    // Prepare local min lvt computation
    local_gvt_manager_->initialize(num_worker_threads_);

    termination_manager_->initialize(num_worker_threads_);

    tw_stats_->initialize(num_worker_threads_, num_local_objects_);

    // Send local initial events and enqueue remote initial events
    partition_id = 0;
    for (auto& partition : objects) {
        if (partition_id == comm_manager_->getID()) {
            for (auto& ob : partition) {
                auto new_events = ob->createInitialEvents();
                sendEvents(new_events, local_object_id_by_name_[ob->name_], ob);
            }
        }
        partition_id++;
    }

    // Send and receive remote initial events
    sendRemoteEvents();
    comm_manager_->waitForAllProcesses();

    std::this_thread::sleep_for(std::chrono::seconds(1));

    comm_manager_->dispatchReceivedMessages();
    comm_manager_->waitForAllProcesses();
}

FileStream& TimeWarpEventDispatcher::getFileStream(SimulationObject *object,
    const std::string& filename, std::ios_base::openmode mode, std::shared_ptr<Event> this_event) {

    unsigned int local_object_id = local_object_id_by_name_[object->name_];

    return *twfs_manager_->getFileStream(filename, mode, local_object_id, this_event);
}

void TimeWarpEventDispatcher::enqueueRemoteEvent(std::shared_ptr<Event> event,
    unsigned int receiver_id) {

    remote_event_queue_lock_.lock();
    // NOTE: sendEventUpdateState must be called with remote_event_queue_lock_
    MatternColor color = mattern_gvt_manager_->sendEventUpdateState(event->timestamp());
    if (event->timestamp() <= max_sim_time_) {
        remote_event_queue_.push_back(std::make_tuple(event, receiver_id, color));
    }
    remote_event_queue_lock_.unlock();
}

void TimeWarpEventDispatcher::sendRemoteEvents() {
    remote_event_queue_lock_.lock();
    while (!remote_event_queue_.empty()) {
        auto event_tuple = std::move(remote_event_queue_.front());
        remote_event_queue_.pop_front();

        auto event = std::get<0>(event_tuple);
        unsigned int receiver_id = std::get<1>(event_tuple);
        MatternColor color = std::get<2>(event_tuple);

        auto event_msg = make_unique<EventMessage>(comm_manager_->getID(), receiver_id,
            event, color);

        comm_manager_->sendMessage(std::move(event_msg));
    }
    remote_event_queue_lock_.unlock();
}

} // namespace warped


