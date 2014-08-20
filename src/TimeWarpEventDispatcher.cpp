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
    std::shared_ptr<TimeWarpCommunicationManager> comm_manager,
    std::unique_ptr<TimeWarpEventSet> event_set,
    std::unique_ptr<TimeWarpGVTManager> gvt_manager,
    std::unique_ptr<TimeWarpStateManager> state_manager,
    std::unique_ptr<TimeWarpOutputManager> output_manager,
    std::unique_ptr<TimeWarpFileStreamManager> twfs_manager) :
        EventDispatcher(max_sim_time), num_worker_threads_(num_worker_threads),
        comm_manager_(comm_manager), event_set_(std::move(event_set)),
        gvt_manager_(std::move(gvt_manager)), state_manager_(std::move(state_manager)),
        output_manager_(std::move(output_manager)), twfs_manager_(std::move(twfs_manager)) {}

void TimeWarpEventDispatcher::startSimulation(const std::vector<std::vector<SimulationObject*>>&
                                              objects) {
    initialize(objects);

    thread_id = num_worker_threads_;
    unused<unsigned int>(std::move(thread_id));

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
            started_min_lvt = true;
        }
        if (GVT_UPDATE(msg_flags)) {
            fossilCollect(gvt_manager_->getGVT());
            msg_flags &= ~MessageFlags::GVTUpdate;
            dynamic_cast<TimeWarpMatternGVTManager*>(gvt_manager_.get())->reset();
        }

        if (calculate_gvt && comm_manager_->getID() == 0) {
            // This will only return true if a token is not in circulation and we need to
            // start another one.
            MessageFlags flags = gvt_manager_->calculateGVT();
            msg_flags |= flags;
            if (PENDING_MATTERN_TOKEN(msg_flags)) {
                min_lvt_flag_.store(num_worker_threads_);
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
            dynamic_cast<TimeWarpMatternGVTManager*>(gvt_manager_.get())->sendMatternGVTToken(
                local_min_lvt);
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

        std::shared_ptr<Event> event = nullptr; //TODO, get next event
        if (event != nullptr) {

            unsigned int current_object_id = local_object_id_by_name_[event->receiverName()];
            SimulationObject* current_object = objects_by_name_[event->receiverName()];

            // Check to see if straggler and rollback if necessary
            if (event->timestamp() < object_simulation_time_[current_object_id]) {
                rollback(event->timestamp(), current_object_id, current_object);
            }

            // Handle a negative event
            if (event->event_type_ == EventType::NEGATIVE) {
//                event_set_->handleAntiMessage(current_object_id, std::move(event));
                continue;
            }

            if (local_min_lvt_flag_[thread_id] > 0 && !calculated_min_flag_[thread_id]) {
                min_lvt_[thread_id] = std::min(send_min_[thread_id], event->timestamp());
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

                output_manager_->insertEvent(e, current_object_id);

                if (node_id == comm_manager_->getID()) {
                    // Local event
                    sendLocalEvent(e);
                } else {
                    // Remote event
                    enqueueRemoteEvent(e, node_id);
                }
            }
        }
    }
}

MessageFlags TimeWarpEventDispatcher::receiveEventMessage(std::unique_ptr<TimeWarpKernelMessage>
    kmsg) {
    auto msg = unique_cast<TimeWarpKernelMessage, EventMessage>(std::move(kmsg));
    dynamic_cast<TimeWarpMatternGVTManager*>(gvt_manager_.get())->receiveEventUpdate(
        msg->gvt_mattern_color);

    unsigned int receiver_object_id = local_object_id_by_name_[msg->event->receiverName()];
    unused<unsigned int>(std::move(receiver_object_id));
//    event_set_->insertEvent(receiver_object_id, std::move(msg->event));

    return MessageFlags::None;
}

void TimeWarpEventDispatcher::sendLocalEvent(std::shared_ptr<Event> event) {
    unsigned int receiver_object_id = local_object_id_by_name_[event->receiverName()];
    unused<unsigned int>(std::move(receiver_object_id));
    //event_set_->insertEvent(receiver_object_id, event);

    if (min_lvt_flag_.load() > 0 && !calculated_min_flag_[thread_id]) {
        unsigned int send_min = send_min_[thread_id];
        send_min_[thread_id] = std::min(send_min, event->timestamp());
    }
}

void TimeWarpEventDispatcher::fossilCollect(unsigned int gvt) {
    twfs_manager_->fossilCollectAll(gvt);
    state_manager_->fossilCollectAll(gvt);
    output_manager_->fossilCollectAll(gvt);
//    event_set_->fossilCollectAll(gvt);
}

void TimeWarpEventDispatcher::cancelEvents(
    std::unique_ptr<std::vector<std::shared_ptr<Event>>> events_to_cancel) {

    if (events_to_cancel->empty()) {
        return;
    }

    do {
        auto event = events_to_cancel->back();

        auto neg_event = std::make_shared<NegativeEvent>(event->receiverName(), event->senderName(),
            event->timestamp());

        events_to_cancel->pop_back();
        unsigned int receiver_id = object_node_id_by_name_[event->receiverName()];
        if (receiver_id != comm_manager_->getID()) {
            enqueueRemoteEvent(neg_event, receiver_id);
        } else {
            sendLocalEvent(neg_event);
        }
    } while (!events_to_cancel->empty());
}

void TimeWarpEventDispatcher::rollback(unsigned int straggler_time, unsigned int local_object_id,
    SimulationObject* object) {

    twfs_manager_->rollback(straggler_time, local_object_id);

    unsigned int restored_timestamp = state_manager_->restoreState(straggler_time, local_object_id,
        object);
    auto events_to_cancel = output_manager_->rollback(straggler_time, local_object_id);

    if (events_to_cancel != nullptr) {
        cancelEvents(std::move(events_to_cancel));
    }

//    event_set_->rollback(local_object_id, restored_timestamp);

    coastForward(restored_timestamp, straggler_time);
}

void TimeWarpEventDispatcher::coastForward(unsigned int start_time, unsigned int stop_time) {
    unsigned int current_time = start_time;
    while (current_time < stop_time) {
        std::shared_ptr<Event> event = nullptr; //TODO, get next event

        unsigned int current_object_id = local_object_id_by_name_[event->receiverName()];
        SimulationObject* current_object = objects_by_name_[event->receiverName()];

        object_simulation_time_[current_object_id] = event->timestamp();
        twfs_manager_->setObjectCurrentTime(event->timestamp(), current_object_id);

        current_object->receiveEvent(*event);
        state_manager_->saveState(event->timestamp(), current_object_id, current_object);
    }
}

void TimeWarpEventDispatcher::initialize(const std::vector<std::vector<SimulationObject*>>&
    objects) {

    unsigned int partition_id = 0;
    for (auto& partition : objects) {
        unsigned int object_id = 0;
        for (auto ob : partition) {
            events_->push(ob->createInitialEvents());
            if (partition_id == comm_manager_->getID()) {
                objects_by_name_[ob->name_] = ob;
                local_object_id_by_name_[ob->name_] = object_id++;
            }
            object_node_id_by_name_[ob->name_] = partition_id;
        }
        partition_id++;
    }

    unsigned int num_local_objects = objects[comm_manager_->getID()].size();
    object_simulation_time_ = make_unique<unsigned int []>(num_local_objects);
    std::memset(object_simulation_time_.get(), 0, num_local_objects*sizeof(unsigned int));

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
}

unsigned int TimeWarpEventDispatcher::getMinimumLVT() {
    unsigned int min = std::numeric_limits<unsigned int>::max();;
    for (unsigned int i = 0; i < num_worker_threads_; i++) {
        min = std::min(min, min_lvt_[i]);
        // Reset send_min back to very large number for next calculation
        send_min_[i] = std::numeric_limits<unsigned int>::max();
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


