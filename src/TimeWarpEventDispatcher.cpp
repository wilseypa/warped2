#include "TimeWarpEventDispatcher.hpp"

#include <atomic>
#include <memory>
#include <string>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>
#include <cmath>
#include <algorithm>    // for std::min

#include "Event.hpp"
#include "EventDispatcher.hpp"
#include "LTSFQueue.hpp"
#include "Partitioner.hpp"
#include "SimulationObject.hpp"
#include "MPICommunicationManager.hpp"
#include "MatternGVTManager.hpp"
#include "StateManager.hpp"
#include "OutputManager.hpp"
#include "EventMessage.hpp"
#include "utility/memory.hpp"

namespace warped {

TimeWarpEventDispatcher::TimeWarpEventDispatcher(unsigned int max_sim_time,
    unsigned int num_worker_threads,
    std::shared_ptr<CommunicationManager> comm_manager,
    std::unique_ptr<LTSFQueue> events, std::unique_ptr<GVTManager> gvt_manager,
    std::unique_ptr<StateManager> state_manager, std::unique_ptr<OutputManager> output_manager) :
        EventDispatcher(max_sim_time), num_worker_threads_(num_worker_threads),
        comm_manager_(comm_manager), events_(std::move(events)),
        gvt_manager_(std::move(gvt_manager)), state_manager_(std::move(state_manager)),
        output_manager_(std::move(output_manager)) {}

void TimeWarpEventDispatcher::startSimulation(const std::vector<std::vector<SimulationObject*>>&
                                              objects) {
    initialize(objects);

    // Create worker threads
    std::vector<std::thread> threads;
    for (unsigned int i = 0; i < std::thread::hardware_concurrency() - 1; ++i) {
        threads.push_back(std::thread {&TimeWarpEventDispatcher::processEvents, this});
    }

    // Flag that says we need to send a mattern token, either to start token circulation
    // or because we have received a token that we must pass along.
    bool pending_mattern_token = false;

    // Flag that says we have started calculating minimum lvt of the objects on this node
    bool started_min_lvt = false;

    // Master thread main loop
    while (gvt_manager_->getGVT() < max_sim_time_) {

        bool flag = comm_manager_->dispatchMessages();
        // We may already have a pending token to send
        pending_mattern_token |= flag;
        if (pending_mattern_token && (min_lvt_flag_.load() == 0) && !started_min_lvt) {
            min_lvt_flag_.store(num_worker_threads_);
            started_min_lvt = true;
        }

        if (calculate_gvt_.load() && comm_manager_->getID() == 0) {
            // This will only return true if a token is not in circulation and we need to
            // start another one.
            pending_mattern_token = gvt_manager_->calculateGVT();

            if (pending_mattern_token == true) {
                min_lvt_flag_.store(num_worker_threads_);
                started_min_lvt = true;
                calculate_gvt_.store(false);
            }
        }

        if (pending_mattern_token && (min_lvt_flag_.load() == 0) && started_min_lvt) {
            auto mattern_gvt_manager = static_cast<MatternGVTManager*>(gvt_manager_.get());
            unsigned int local_min_lvt = getMinimumLVT();
            mattern_gvt_manager->sendMatternGVTToken(local_min_lvt);
            pending_mattern_token = false;
            started_min_lvt = false;
        }

        // This sends all messages that are in the send buffer
        comm_manager_->sendAllMessages();
    }

    comm_manager_->finalize();
}

void TimeWarpEventDispatcher::processEvents() {

}

bool TimeWarpEventDispatcher::receiveEventMessage(std::unique_ptr<KernelMessage> kmsg) {
    auto msg = unique_cast<KernelMessage, EventMessage>(std::move(kmsg));
    gvt_manager_->setGvtInfo(msg->gvt_mattern_color);

   // TODO This is incomplete
   return false;
}

void TimeWarpEventDispatcher::sendRemoteEvent(std::unique_ptr<Event> event,
    unsigned int receiver_id) {

    unsigned int sender_id = comm_manager_->getID();
    int color = gvt_manager_->getGvtInfo(event->timestamp());

    auto event_msg = make_unique<EventMessage>(sender_id, receiver_id, std::move(event), color);
    comm_manager_->enqueueMessage(std::move(event_msg));
}

void TimeWarpEventDispatcher::sendLocalEvent(std::unique_ptr<Event> event, unsigned int thread_id) {
    // TODO, totally incomplete still, just added some probably temporary gvt stuff

    if (min_lvt_flag_.load() > 0 && !calculated_min_lvt_by_thread_id_[thread_id]) {
        unsigned int send_min = send_min_by_thread_id_[thread_id];
        send_min_by_thread_id_[thread_id] = std::min(send_min, event->timestamp());
    }
}

void TimeWarpEventDispatcher::fossilCollect(unsigned int gvt) {
    state_manager_->fossilCollectAll(gvt);
    output_manager_->fossilCollectAll(gvt);

    //TODO still incomplete
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
            objects_by_name_[ob->name_] = ob;
            local_object_id_by_name_[ob->name_] = object_id++;
            object_node_id_by_name_[ob->name_] = partition_id;
        }
        partition_id++;
    }

    unsigned int num_local_objects = objects[comm_manager_->getID()].size();

    // Creates the state queues and output queues
    state_manager_->initialize(num_local_objects);
    output_manager_->initialize(num_local_objects);


    // Register message handlers
    gvt_manager_->initialize();

    std::function<bool(std::unique_ptr<KernelMessage>)> handler =
        std::bind(&TimeWarpEventDispatcher::receiveEventMessage, this,
        std::placeholders::_1);
    comm_manager_->addMessageHandler(MessageType::EventMessage, handler);

    min_lvt_by_thread_id_ = make_unique<unsigned int []>(num_worker_threads_);
    send_min_by_thread_id_ = make_unique<unsigned int []>(num_worker_threads_);
    calculated_min_lvt_by_thread_id_ = make_unique<bool []>(num_worker_threads_);
}

unsigned int TimeWarpEventDispatcher::getMinimumLVT() {
    unsigned int min;
    for (unsigned int i = 0; i < num_worker_threads_; i++) {
        min = std::min(min, min_lvt_by_thread_id_[i]);
    }
    return min;
}

} // namespace warped


