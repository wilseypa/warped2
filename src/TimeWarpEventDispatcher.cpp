#include "TimeWarpEventDispatcher.hpp"

#include <atomic>
#include <memory>
#include <string>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

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

    for (auto& partition : objects) {
        for (auto& ob : partition) {
            events_->push(ob->createInitialEvents());
            objects_by_name_[ob->name_] = ob;
            object_id_by_name_[ob->name_] = num_objects_;
            num_objects_++;
        }
    }

    objects_per_partition_ = num_objects_/comm_manager_->getNumProcesses();

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

    for (auto& t : threads) {
        t.join();
    }

    finalizeCommunicationManager();
}

void TimeWarpEventDispatcher::processEvents() {

}

void TimeWarpEventDispatcher::receiveMessage(std::unique_ptr<KernelMessage> msg) {
    EventMessage *e = dynamic_cast<EventMessage*>(msg.get());
    std::unique_ptr<EventMessage> event_msg;
    if(e != nullptr)
    {
        msg.release();
        event_msg.reset(e);
    }

    gvt_manager_->setGvtInfo(event_msg->gvt_mattern_color);

    //TODO This is incomplete
}

void TimeWarpEventDispatcher::sendRemoteEvent(std::unique_ptr<Event> event) {
    unsigned int sender_id = comm_manager_->getID();
    unsigned int receiver_id =
        object_id_by_name_[event->receiverName()]/objects_per_partition_;
    int color = gvt_manager_->getGvtInfo(event->timestamp());

    auto event_msg = make_unique<EventMessage>(sender_id, receiver_id, std::move(event), color);
    comm_manager_->enqueueMessage(std::move(event_msg));
}

// TODO double check to make sure this will work
bool TimeWarpEventDispatcher::isObjectLocal(std::string object_name) {
    unsigned int object_id = object_id_by_name_[object_name];
    return object_id/objects_per_partition_ == comm_manager_->getID();
}

void TimeWarpEventDispatcher::fossilCollect(unsigned int gvt) {
    state_manager_->fossilCollectAll(gvt);
    output_manager_->fossilCollectAll(gvt);

    //TODO still incomplete
}

void TimeWarpEventDispatcher::rollback(unsigned int straggler_time, unsigned int local_object_id) {
    unsigned int retored_timestamp = state_manager_->restoreState(straggler_time, local_object_id);
    auto events_to_canel = output_manager_->rollback(straggler_time, local_object_id);

    //TODO this in incomplete, the restored_timestamp is the timestamp of the state of the
    // object that has been restored. All unprocessed event before or equal to this time must be
    // regenerated. events_to_canel is a vector that contains event that must be sent as
    // anti-messages.
}

} // namespace warped


