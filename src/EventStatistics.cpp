#include "EventStatistics.hpp"

#include <fstream>
#include <iostream>
#include <string>

namespace warped {

void EventStatistics::writeToFile(const std::string& filename) {
    std::ofstream ofs(filename, std::ofstream::out);
    ofs << *this;
}

void EventStatistics::record(const std::string& source, unsigned int send_time,
                             const std::vector<std::unique_ptr<Event>>& events) {
    for (const auto& e : events) {
        record(source, send_time, e.get());
    }
}

std::ostream& operator<<(std::ostream& stream, const EventStatistics& stats) {
    return stats.printStats(stream);
}

} // namespace warped
