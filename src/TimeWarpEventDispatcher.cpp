#include "TimeWarpEventDispatcher.hpp"

#include <atomic>
#include <memory>
#include <string>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>
#include <cmath>

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
    std::unique_ptr<LTSFQueue> events, std::unique_ptr<GVTManager> gvt_manager,
    std::unique_ptr<StateManager> state_manager, std::unique_ptr<OutputManager> output_manager) :
        EventDispatcher(max_sim_time), events_(std::move(events)),
        gvt_manager_(std::move(gvt_manager)), state_manager_(std::move(state_manager)),
        output_manager_(std::move(output_manager)) {}

// This function implementation is merely a speculation for how the this
// function might work in the future. It's definitely incomplete, and may not
// be the best approach at all.
void TimeWarpEventDispatcher::startSimulation(const std::vector<std::vector<SimulationObject*>>&
                                              objects) {
    initialize(objects);

    // Create worker threads
    std::vector<std::thread> threads;
    for (unsigned int i = 0; i < std::thread::hardware_concurrency() - 1; ++i) {
        threads.push_back(std::thread {&TimeWarpEventDispatcher::processEvents, this});
    }

    // Master thread main loop
    while (gvt_manager_->getGVT() < max_sim_time_) {

        // Node with id 0 will be the only one to initiate gvt calculation
        if (comm_manager_->getID() == 0) {
            gvt_manager_->calculateGVT();
        }

        // This sends all messages that are in the send buffer
        comm_manager_->sendAllMessages();
    }

    finalizeCommunicationManager();
}

void TimeWarpEventDispatcher::processEvents() {

}

void TimeWarpEventDispatcher::receiveMessage(std::unique_ptr<KernelMessage> msg) {
    std::unique_ptr<EventMessage> event_msg = unique_cast<KernelMessage, EventMessage>
        (std::move(msg));

    gvt_manager_->setGvtInfo(event_msg->gvt_mattern_color);

   // TODO This is incomplete
}

void TimeWarpEventDispatcher::sendRemoteEvent(std::unique_ptr<Event> event) {
    unsigned int sender_id = comm_manager_->getID();
    unsigned int receiver_id = 0;
    int color = gvt_manager_->getGvtInfo(event->timestamp());

    auto event_msg = make_unique<EventMessage>(sender_id, receiver_id, std::move(event), color);
    comm_manager_->enqueueMessage(std::move(event_msg));
}

void TimeWarpEventDispatcher::fossilCollect(unsigned int gvt) {
    state_manager_->fossilCollectAll(gvt);
    output_manager_->fossilCollectAll(gvt);

    //TODO still incomplete
}

void TimeWarpEventDispatcher::rollback(unsigned int straggler_time, unsigned int local_object_id,
    SimulationObject* object) {
    unsigned int restored_timestamp = state_manager_->restoreState(straggler_time, local_object_id,
        object);
    auto events_to_cancel = output_manager_->rollback(straggler_time, local_object_id);

    // TODO temporary so compiler wouldn't complain
    restored_timestamp = 1;
    events_to_cancel->size();

    //TODO this in incomplete, the restored_timestamp is the timestamp of the state of the
    // object that has been restored. All unprocessed event before or equal to this time must be
    // regenerated. events_to_canel is a vector that contains event that must be sent as
    // anti-messages.
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
}

} // namespace warped


