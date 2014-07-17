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

#include "Event.hpp"
#include "EventDispatcher.hpp"
#include "LTSFQueue.hpp"
#include "Partitioner.hpp"
#include "SimulationObject.hpp"
#include "TimeWarpMPICommunicationManager.hpp"
#include "TimeWarpMatternGVTManager.hpp"
#include "TimeWarpStateManager.hpp"
#include "TimeWarpOutputManager.hpp"
#include "utility/memory.hpp"

WARPED_REGISTER_POLYMORPHIC_SERIALIZABLE_CLASS(warped::EventMessage)

namespace warped {

TimeWarpEventDispatcher::TimeWarpEventDispatcher(unsigned int max_sim_time,
    unsigned int num_worker_threads,
    std::shared_ptr<TimeWarpCommunicationManager> comm_manager,
    std::unique_ptr<LTSFQueue> events, std::unique_ptr<TimeWarpGVTManager> gvt_manager,
    std::unique_ptr<TimeWarpStateManager> state_manager,
    std::unique_ptr<TimeWarpOutputManager> output_manager) :
        EventDispatcher(max_sim_time), num_worker_threads_(num_worker_threads),
        comm_manager_(comm_manager), events_(std::move(events)),
        gvt_manager_(std::move(gvt_manager)), state_manager_(std::move(state_manager)),
        output_manager_(std::move(output_manager)) {}

void TimeWarpEventDispatcher::startSimulation(const std::vector<std::vector<SimulationObject*>>&
                                              objects) {
    initialize(objects);

    unsigned int thread_id;
    thread_id = num_worker_threads_;
    // Create worker threads
    std::vector<std::thread> threads;
    for (unsigned int i = 0; i < num_worker_threads_; ++i) {
        threads.push_back(std::thread {&TimeWarpEventDispatcher::processEvents, this, i});
    }

    MessageFlags msg_flags = MessageFlags::None;

    // Flag that says we have started calculating minimum lvt of the objects on this node
    bool started_min_lvt = false;

    // Master thread main loop
    while (gvt_manager_->getGVT() < max_sim_time_) {

        MessageFlags flags = comm_manager_->dispatchMessages();
        // We may already have a pending token to send
        msg_flags |= flags;
        if (PENDING_MATTERN_TOKEN(msg_flags) && (min_lvt_flag_.load() == 0) && !started_min_lvt) {
            min_lvt_flag_.store(num_worker_threads_);
            started_min_lvt = true;
        }
        if (GVT_UPDATE(msg_flags)) {
            fossilCollect(gvt_manager_->getGVT());
            msg_flags &= ~MessageFlags::GVTUpdate;
        }

        if (calculate_gvt_.load() && comm_manager_->getID() == 0) {
            // This will only return true if a token is not in circulation and we need to
            // start another one.
            MessageFlags flags = gvt_manager_->calculateGVT();
            msg_flags |= flags;
            if (PENDING_MATTERN_TOKEN(msg_flags)) {
                min_lvt_flag_.store(num_worker_threads_);
                started_min_lvt = true;
                calculate_gvt_.store(false);
            }
        }

        if (PENDING_MATTERN_TOKEN(msg_flags) && (min_lvt_flag_.load() == 0) && started_min_lvt) {
            auto mattern_gvt_manager = static_cast<TimeWarpMatternGVTManager*>(gvt_manager_.get());
            unsigned int local_min_lvt = getMinimumLVT();
            mattern_gvt_manager->sendMatternGVTToken(local_min_lvt);
            msg_flags &= ~MessageFlags::PendingMatternToken;
            started_min_lvt = false;
        }

        // This sends all messages that are in the send buffer
        comm_manager_->sendAllMessages();
    }

    comm_manager_->finalize();
}

void TimeWarpEventDispatcher::processEvents(unsigned int id) {
    unsigned int thread_id;
    thread_id = id;

    local_min_lvt_flag_[thread_id] = min_lvt_flag_.load();
    // Get event and handle rollback if necessary

    unsigned int min_lvt_of_this_thread = 0;

    if (local_min_lvt_flag_[thread_id] > 0 && !calculated_min_flag_[thread_id]) {
        min_lvt_[thread_id] = std::min(send_min_[thread_id], min_lvt_of_this_thread);
        min_lvt_flag_--;
    }
}

MessageFlags TimeWarpEventDispatcher::receiveEventMessage(std::unique_ptr<TimeWarpKernelMessage>
    kmsg) {
    auto msg = unique_cast<TimeWarpKernelMessage, EventMessage>(std::move(kmsg));
    gvt_manager_->setGvtInfo(msg->gvt_mattern_color);

   // TODO This is incomplete
   return MessageFlags::None;
}

void TimeWarpEventDispatcher::sendRemoteEvent(std::unique_ptr<Event> event,
    unsigned int receiver_id) {

    int color = gvt_manager_->getGvtInfo(event->timestamp());

    auto event_msg = make_unique<EventMessage>(receiver_id, std::move(event),
        color);

    comm_manager_->enqueueMessage(std::move(event_msg));
}

void TimeWarpEventDispatcher::sendLocalEvent(std::unique_ptr<Event> event, unsigned int thread_id) {
    // TODO, totally incomplete still, just added some probably temporary gvt stuff

    if (min_lvt_flag_.load() > 0 && !calculated_min_flag_[thread_id]) {
        unsigned int send_min = send_min_[thread_id];
        send_min_[thread_id] = std::min(send_min, event->timestamp());
    }
}

void TimeWarpEventDispatcher::fossilCollect(unsigned int gvt) {
    twfs_manager_->fossilCollectAll(gvt);
    state_manager_->fossilCollectAll(gvt);
    output_manager_->fossilCollectAll(gvt);

    //TODO still incomplete, input queues?
}

void TimeWarpEventDispatcher::cancelEvents(
    std::unique_ptr<std::vector<std::unique_ptr<Event>>> events_to_cancel) {

    if (events_to_cancel->empty()) {
        return;
    }

    do {
        auto event = std::move(events_to_cancel->back());
        events_to_cancel->pop_back();
        unsigned int receiver_id = object_node_id_by_name_[event->receiverName()];
        if (receiver_id != comm_manager_->getID()) {
            sendRemoteEvent(std::move(event), receiver_id);
        } else {
            sendLocalEvent(std::move(event), 0); // TODO get thread_id of sender
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

    //TODO this in incomplete, the restored_timestamp is the timestamp of the state of the
    // object that has been restored. All unprocessed event before or equal to this time must be
    // regenerated.
    restored_timestamp = 1;
}

void TimeWarpEventDispatcher::initialize(const std::vector<std::vector<SimulationObject*>>&
    objects) {

    unsigned int partition_id = 0;
    for (auto& partition : objects) {
        unsigned int object_id = 0;
        for (auto& ob : partition) {
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

    // Creates the state queues, output queues, and filestream queues for each local object
    state_manager_->initialize(num_local_objects);
    output_manager_->initialize(num_local_objects);
    twfs_manager_->initialize(num_local_objects);

    // Register message handlers
    gvt_manager_->initialize();

    std::function<MessageFlags(std::unique_ptr<TimeWarpKernelMessage>)> handler =
        std::bind(&TimeWarpEventDispatcher::receiveEventMessage, this,
        std::placeholders::_1);
    comm_manager_->addMessageHandler(MessageType::EventMessage, handler);

    // Prepare local min lvt computation
    min_lvt_ = make_unique<unsigned int []>(num_worker_threads_);
    send_min_ = make_unique<unsigned int []>(num_worker_threads_);
    calculated_min_flag_ = make_unique<bool []>(num_worker_threads_);
}

unsigned int TimeWarpEventDispatcher::getMinimumLVT() {
    unsigned int min;
    for (unsigned int i = 0; i < num_worker_threads_; i++) {
        min = std::min(min, min_lvt_[i]);
        // Reset send_min back to very large number for next calculation
        send_min_[i] = std::numeric_limits<unsigned int>::max();
    }
    return min;
}

FileStream& TimeWarpEventDispatcher::getFileStream(
    SimulationObject *object, const std::string& filename, std::ios_base::openmode mode) {

    unsigned int local_object_id = local_object_id_by_name_[object->name_];
    TimeWarpFileStream* twfs = twfs_manager_->getFileStream(filename, mode, local_object_id);

    return *twfs;
}

} // namespace warped


