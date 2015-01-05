#include "TimeWarpEventDispatcher.hpp"

#include <atomic>
#include <memory>
#include <string>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>
#include <cmath>
#include <limits> // for std::numeric_limits<>::max();
#include <algorithm>    // for std::min
#include <chrono>   // for std::chrono::steady_clock
#include <cstring> // for std::memset
#include <iostream>
#include <cassert>

#include "Event.hpp"
#include "EventDispatcher.hpp"
#include "LTSFQueue.hpp"
#include "Partitioner.hpp"
#include "SimulationObject.hpp"
#include "TimeWarpMPICommunicationManager.hpp"
#include "TimeWarpMatternGVTManager.hpp"
#include "TimeWarpStateManager.hpp"
#include "TimeWarpOutputManager.hpp"
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
    std::unique_ptr<TimeWarpGVTManager> gvt_manager,
    std::unique_ptr<TimeWarpStateManager> state_manager,
    std::unique_ptr<TimeWarpOutputManager> output_manager,
    std::unique_ptr<TimeWarpFileStreamManager> twfs_manager) :
        EventDispatcher(max_sim_time), num_worker_threads_(num_worker_threads),
        num_schedulers_(num_schedulers), comm_manager_(comm_manager), 
        event_set_(std::move(event_set)), gvt_manager_(std::move(gvt_manager)), 
        state_manager_(std::move(state_manager)), 
        output_manager_(std::move(output_manager)), twfs_manager_(std::move(twfs_manager)) {}

void TimeWarpEventDispatcher::startSimulation(const std::vector<std::vector<SimulationObject*>>&
                                              objects) {
    initialize(objects);
    comm_manager_->waitForAllProcesses();

    // Create worker threads
    std::vector<std::thread> threads;
    for (unsigned int i = 0; i < num_worker_threads_; ++i) {
        auto thread(std::thread {&TimeWarpEventDispatcher::processEvents, this, i});
        thread.detach();
        threads.push_back(std::move(thread));
    }

    MessageFlags msg_flags = MessageFlags::None;

    auto gvt_start = std::chrono::steady_clock::now();
    bool calculate_gvt = false;

    // Flag that says we have started calculating minimum lvt of the objects on this node
    bool started_min_lvt = false;

    // Master thread main loop
    while (gvt_manager_->getGVT() < max_sim_time_) {

        MessageFlags flags = comm_manager_->dispatchReceivedMessages();
        // We may already have a pending token to send
        msg_flags |= flags;
        if (PENDING_MATTERN_TOKEN(msg_flags) && (min_lvt_flag_.load() == 0) && !started_min_lvt) {
            min_lvt_flag_.store(num_worker_threads_);
            event_set_->gvtCalcRequest();
            started_min_lvt = true;
        }
        if (GVT_UPDATE(msg_flags)) {
            //fossilCollect(gvt_manager_->getGVT());
            msg_flags &= ~MessageFlags::GVTUpdate;
            dynamic_cast<TimeWarpMatternGVTManager*>(gvt_manager_.get())->reset();
        }

        if (calculate_gvt && comm_manager_->getID() == 0) {
            // This will only return true if a token is not in circulation and we need to
            // start another one.
            MessageFlags flags = gvt_manager_->calculateGVT();
            msg_flags |= flags;
            if (PENDING_MATTERN_TOKEN(msg_flags) && (min_lvt_flag_.load() == 0) && !started_min_lvt ) {
                min_lvt_flag_.store(num_worker_threads_);
                event_set_->gvtCalcRequest();
                started_min_lvt = true;
                calculate_gvt = false;
            }
        } else if (comm_manager_->getID() == 0) {
            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>
                (now - gvt_start).count();
            if (elapsed >= gvt_manager_->getGVTPeriod()) {
                calculate_gvt = true;
                gvt_start = now;
            }
        }

        if (PENDING_MATTERN_TOKEN(msg_flags) && (min_lvt_flag_.load() == 0) && started_min_lvt) {
            unsigned int local_min_lvt = getMinimumLVT();
            if (comm_manager_->getNumProcesses() > 1) {
                dynamic_cast<TimeWarpMatternGVTManager*>(gvt_manager_.get())->sendMatternGVTToken(
                    local_min_lvt);
            } else {
                gvt_manager_->setGVT(local_min_lvt);
                std::cout << "GVT: " << local_min_lvt << std::endl;
                //fossilCollect(gvt_manager_->getGVT());
                dynamic_cast<TimeWarpMatternGVTManager*>(gvt_manager_.get())->reset();
            }
            msg_flags &= ~MessageFlags::PendingMatternToken;
            started_min_lvt = false;
        }

        sendRemoteEvents();
    }

    comm_manager_->finalize();
}

void TimeWarpEventDispatcher::processEvents(unsigned int id) {
    thread_id = id;

    while (gvt_manager_->getGVT() < max_sim_time_) {
        local_min_lvt_flag_[thread_id] = min_lvt_flag_.load();

        std::shared_ptr<Event> event = event_set_->getEvent(thread_id);
        if (event != nullptr) {

            unsigned int current_object_id = local_object_id_by_name_[event->receiverName()];
            SimulationObject* current_object = objects_by_name_[event->receiverName()];

            // Check to see if straggler event and rollback if true
            if ((prev_processed_event_[current_object_id]) && 
                    ((*event < *prev_processed_event_[current_object_id]) || 
                     ((*event == *prev_processed_event_[current_object_id]) && 
                      (event->event_type_ == EventType::NEGATIVE)))) {

                std::cout << "roll" << std::endl;
                rollback(event, current_object_id, current_object);
                rollback_count_++;
                if (event->event_type_ == EventType::NEGATIVE) {
                    event_set_->startScheduling(current_object_id);
                    continue;
                }
            }

            assert(event->event_type_ != EventType::NEGATIVE);

            if ((local_min_lvt_flag_[thread_id] > 0 && !calculated_min_flag_[thread_id])
                    /*&& !event_set_->isRollbackPending()*/) {
                min_lvt_[thread_id] = std::min(send_min_[thread_id], event->timestamp());
                calculated_min_flag_[thread_id] = true;
                min_lvt_flag_--;
            }

            // Update simulation time
            object_simulation_time_[current_object_id] = event->timestamp();
            twfs_manager_->setObjectCurrentTime(event->timestamp(), current_object_id);

            // process event and get new events
            auto new_events = current_object->receiveEvent(*event);

            // Save state
            state_manager_->saveState(event->timestamp(), current_object_id, current_object);

            // Send new events
            for (auto& e: new_events) {
                unsigned int node_id = object_node_id_by_name_[event->receiverName()];
                e->sender_name_ = current_object->name_;
                e->rollback_cnt_ = rollback_count_;
                output_manager_->insertEvent(e, current_object_id);

                assert(e->timestamp() >= object_simulation_time_[current_object_id]);

                if (node_id == comm_manager_->getID()) {
                    // Local event
                    sendLocalEvent(e);
                } else {
                    // Remote event
                    enqueueRemoteEvent(e, node_id);
                }
            }

            // Move the next event from object into the schedule queue
            // Also transfer old event to processed queue
            event_set_->replenishScheduler(current_object_id, event);

            // Update previous processed event
            prev_processed_event_[current_object_id] = std::move(event);

        } else {
            // TODO, do something here
            assert(false);
        }
    }
}

MessageFlags TimeWarpEventDispatcher::receiveEventMessage(
        std::unique_ptr<TimeWarpKernelMessage> kmsg) {
    auto msg = unique_cast<TimeWarpKernelMessage, EventMessage>(std::move(kmsg));
    dynamic_cast<TimeWarpMatternGVTManager*>(gvt_manager_.get())->receiveEventUpdate(
        msg->gvt_mattern_color);

    unsigned int receiver_object_id = local_object_id_by_name_[msg->event->receiverName()];
    event_set_->insertEvent(receiver_object_id, msg->event);

    return MessageFlags::None;
}

void TimeWarpEventDispatcher::sendLocalEvent(std::shared_ptr<Event> event) {
    unsigned int receiver_object_id = local_object_id_by_name_[event->receiverName()];
    event_set_->insertEvent(receiver_object_id, event);

    if (min_lvt_flag_.load() > 0 && !calculated_min_flag_[thread_id]) {
        unsigned int send_min = send_min_[thread_id];
        send_min_[thread_id] = std::min(send_min, event->timestamp());
    }
}

void TimeWarpEventDispatcher::fossilCollect(unsigned int gvt) {
    twfs_manager_->fossilCollectAll(gvt);
    state_manager_->fossilCollectAll(gvt);
    output_manager_->fossilCollectAll(gvt);
    event_set_->fossilCollectAll(gvt);
}

void TimeWarpEventDispatcher::cancelEvents(
    std::unique_ptr<std::vector<std::shared_ptr<Event>>> events_to_cancel) {

    if (events_to_cancel->empty()) {
        return;
    }

    do {
        auto event = events_to_cancel->back();

        auto neg_event = std::make_shared<NegativeEvent>(event);

        events_to_cancel->pop_back();

        unsigned int receiver_node_id = object_node_id_by_name_[event->receiverName()];

        //unsigned int receiver_id = local_object_id_by_name_[event->receiverName()];
        //unsigned int sender_id = local_object_id_by_name_[event->sender_name_];
        //if (sender_id == receiver_id) {
        //    neg_event.reset();
        //    continue;
        //}

        if (receiver_node_id != comm_manager_->getID()) {
            enqueueRemoteEvent(neg_event, receiver_node_id);
        } else {
            sendLocalEvent(neg_event);
        }

    } while (!events_to_cancel->empty());
}

void TimeWarpEventDispatcher::rollback(std::shared_ptr<Event> straggler_event, 
                            unsigned int local_object_id, SimulationObject* object) {

    unsigned int straggler_time = straggler_event->timestamp();
    twfs_manager_->rollback(straggler_time, local_object_id);

    assert(straggler_time >= gvt_manager_->getGVT());

    unsigned int restored_timestamp = 
                    state_manager_->restoreState(straggler_time, local_object_id, object);
    auto events_to_cancel = output_manager_->rollback(straggler_time, local_object_id);

    assert((restored_timestamp < straggler_event->timestamp()) || (restored_timestamp == 0));

    if (events_to_cancel != nullptr) {
        cancelEvents(std::move(events_to_cancel));
    }

    event_set_->rollback(local_object_id, restored_timestamp, straggler_event);

    object_simulation_time_[local_object_id] = restored_timestamp;
    twfs_manager_->setObjectCurrentTime(restored_timestamp, local_object_id);

    coastForward(object, straggler_event);
}

void TimeWarpEventDispatcher::coastForward(SimulationObject* object, 
                                            std::shared_ptr<Event> straggler_event) {

    unsigned int stop_time = straggler_event->timestamp();
    unsigned int current_object_id = local_object_id_by_name_[object->name_];

    auto events = event_set_->getEventsForCoastForward(current_object_id);
    for (auto& event : *events) {
        if (((straggler_event->event_type_ == EventType::NEGATIVE) && 
                (*event == *straggler_event)) || (event->timestamp() > stop_time)) {
            event.reset();
            continue;
        }

        assert(event->timestamp() <= stop_time);
        assert(event->timestamp() >= object_simulation_time_[current_object_id]);
        assert(event->event_type_ != EventType::NEGATIVE);

        object_simulation_time_[current_object_id] = event->timestamp();
        twfs_manager_->setObjectCurrentTime(event->timestamp(), current_object_id);
        object->receiveEvent(*event);
        state_manager_->saveState(event->timestamp(), current_object_id, object);
        event_set_->coastForwardedEvent(current_object_id, event);
    }
}

void TimeWarpEventDispatcher::initialize(
        const std::vector<std::vector<SimulationObject*>>& objects) {
    unsigned int num_local_objects = objects[comm_manager_->getID()].size();
    event_set_->initialize(num_local_objects, num_schedulers_, num_worker_threads_);
    rollback_count_ = 0;

    unsigned int partition_id = 0;
    for (auto& partition : objects) {
        unsigned int object_id = 0;
        for (auto& ob : partition) {
            if (partition_id == comm_manager_->getID()) {
                objects_by_name_[ob->name_] = ob;
                local_object_id_by_name_[ob->name_] = object_id;
                auto new_events = ob->createInitialEvents();
                for (auto& e: new_events) {
                    e->sender_name_ = ob->name_;
                    e->rollback_cnt_ = rollback_count_;
                    event_set_->insertEvent(object_id, e);
                }
                object_id++;
            }
            object_node_id_by_name_[ob->name_] = partition_id;
        }
        partition_id++;
    }

    object_simulation_time_ = make_unique<unsigned int []>(num_local_objects);
    std::memset(object_simulation_time_.get(), 0, num_local_objects*sizeof(unsigned int));

    prev_processed_event_ = make_unique<std::shared_ptr<Event> []>(num_local_objects);
    std::memset(prev_processed_event_.get(), 0, 
                    num_local_objects*sizeof(std::shared_ptr<Event>));

    // Creates the state queues, output queues, and filestream queues for each local object
    state_manager_->initialize(num_local_objects);
    for (auto ob: objects_by_name_) {
        state_manager_->saveState(0, local_object_id_by_name_[ob.first], ob.second);
    }
    output_manager_->initialize(num_local_objects);
    twfs_manager_->initialize(num_local_objects);

    // Register message handlers
    gvt_manager_->initialize();

    std::function<MessageFlags(std::unique_ptr<TimeWarpKernelMessage>)> handler =
        std::bind(&TimeWarpEventDispatcher::receiveEventMessage, this,
        std::placeholders::_1);
    comm_manager_->addRecvMessageHandler(MessageType::EventMessage, handler);

    // Prepare local min lvt computation
    min_lvt_ = make_unique<unsigned int []>(num_worker_threads_);
    send_min_ = make_unique<unsigned int []>(num_worker_threads_);
    calculated_min_flag_ = make_unique<bool []>(num_worker_threads_);
    local_min_lvt_flag_ = make_unique<unsigned int []>(num_worker_threads_);
    for (unsigned int i = 0; i < num_worker_threads_; i++) {
        min_lvt_[i] = std::numeric_limits<unsigned int>::max();
        send_min_[i] = std::numeric_limits<unsigned int>::max();
        calculated_min_flag_[i] = false;
        local_min_lvt_flag_[i] = 0;
    }
}

unsigned int TimeWarpEventDispatcher::getMinimumLVT() {
    unsigned int min = std::numeric_limits<unsigned int>::max();
    for (unsigned int i = 0; i < num_worker_threads_; i++) {
        min = std::min(min, min_lvt_[i]);
        // Reset send_min back to very large number for next calculation
        send_min_[i] = std::numeric_limits<unsigned int>::max();
        calculated_min_flag_[i] = false;
        min_lvt_[i] = std::numeric_limits<unsigned int>::max();
    }
    return min;
}

unsigned int TimeWarpEventDispatcher::getSimulationTime(SimulationObject* object) {
    unsigned int object_id = local_object_id_by_name_[object->name_];
    return object_simulation_time_[object_id];
}

FileStream& TimeWarpEventDispatcher::getFileStream(
    SimulationObject *object, const std::string& filename, std::ios_base::openmode mode) {

    unsigned int local_object_id = local_object_id_by_name_[object->name_];
    TimeWarpFileStream* twfs = twfs_manager_->getFileStream(filename, mode, local_object_id);

    return *twfs;
}

void TimeWarpEventDispatcher::enqueueRemoteEvent(std::shared_ptr<Event> event,
    unsigned int receiver_id) {

    remote_event_queue_lock_.lock();
    remote_event_queue_.push_back(std::make_pair(event, receiver_id));
    remote_event_queue_lock_.unlock();
}

void TimeWarpEventDispatcher::sendRemoteEvents() {
    remote_event_queue_lock_.lock();
    while (!remote_event_queue_.empty()) {
        auto event = std::move(remote_event_queue_.front());
        remote_event_queue_.pop_front();

        MatternColor color = dynamic_cast<TimeWarpMatternGVTManager*>
            (gvt_manager_.get())->sendEventUpdate(event.first->timestamp());

        unsigned int receiver_id = event.second;
        auto event_msg = make_unique<EventMessage>(comm_manager_->getID(), receiver_id,
            event.first, color);

        comm_manager_->sendMessage(std::move(event_msg));
    }
    remote_event_queue_lock_.unlock();
}

} // namespace warped


