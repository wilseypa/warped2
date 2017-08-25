#include "EventStatistics.hpp"

#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace warped {

void EventStatistics::writeToFile() {
    std::ofstream ofs(filename_, std::ofstream::out);
    ofs << *this;
}

void EventStatistics::record(const std::string& source, unsigned int send_time,
                             const std::vector<std::shared_ptr<Event>>& events) {
    if (events.empty()) {
        record(source, send_time, nullptr);
    } else {
        for (const auto& e : events) {
            record(source, send_time, e.get());
        }
    }
}

std::ostream& operator<<(std::ostream& stream, const EventStatistics& stats) {
    return stats.printStats(stream);
}

} // namespace warped
