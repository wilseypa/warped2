#include "SequentialEventDispatcher.hpp"

#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "Event.hpp"
#include "EventDispatcher.hpp"
#include "EventStatistics.hpp"
#include "LogicalProcess.hpp"
#include "STLLTSFQueue.hpp"
#include "utility/memory.hpp"
#include "utility/warnings.hpp"

namespace warped {

SequentialEventDispatcher::SequentialEventDispatcher(unsigned int max_sim_time,
                                                     std::unique_ptr<EventStatistics> stats)
    : EventDispatcher(max_sim_time), stats_(std::move(stats)) {}

void SequentialEventDispatcher::startSimulation(
    const std::vector<std::vector<LogicalProcess*>>& lps) {
    if (lps.size() != 1) {
        throw std::runtime_error(std::string("Sequential simulation only supports 1 partition."));
    }

    std::unordered_map<std::string, LogicalProcess*> lps_by_name;
    STLLTSFQueue events;
    current_sim_time_ = 0;

    for (auto& lp : lps[0]) {
        auto new_events = lp->initializeLP();
        for (auto& e : new_events) {
            e->sender_name_ = lp->name_;
        }
        stats_->record(lp->name_, current_sim_time_, new_events);
        events.push(std::move(new_events));
        lps_by_name[lp->name_] = lp;
    }

    int count = 0;
    while ((current_sim_time_ <= max_sim_time_) && !events.empty()) {
        auto event = events.pop();
        current_sim_time_ = event->timestamp();
        if (current_sim_time_ > max_sim_time_) break;
        auto receiver = lps_by_name[event->receiverName()];
        auto new_events = receiver->receiveEvent(*event.get());
        for (auto& e : new_events) {
            e->sender_name_ = receiver->name_;
        }
        count++;
        stats_->record(event->receiverName(), current_sim_time_, new_events);
        events.push(std::move(new_events));
    }

    std::cout << "Events committed: " << count << std::endl;
    stats_->writeToFile();
}

FileStream& SequentialEventDispatcher::getFileStream(LogicalProcess* lp,
    const std::string& filename, std::ios_base::openmode mode, std::shared_ptr<Event> this_event) {

    unused(this_event);

    if (lp_by_filename_.count(filename) == 0) {
        // Create filestream and insert into filename to stream map
        std::unique_ptr<FileStream, FileStreamDeleter> fs(new FileStream(filename, mode),
            FileStreamDeleter());
        file_stream_by_filename_[filename] = std::move(fs);

        // Insert into filename to lp map
        lp_by_filename_[filename] = lp;

        return *(file_stream_by_filename_[filename].get());

    } else if (lp != lp_by_filename_[filename]) {
        throw std::runtime_error(std::string("A filestream can only be used by a single lp"));
    } else {
        return *(file_stream_by_filename_[filename].get());
    }
}

} // namespace warped
