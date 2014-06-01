#include "IndividualEventStatistics.hpp"

#include <iostream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "json/json.h"

#include "Event.hpp"
#include "utility/memory.hpp"

namespace warped {

IndividualEventStatistics::IndividualEventStatistics(std::string filename)
    : EventStatistics(std::move(filename)),
      root_(make_unique<Json::Value>(Json::ValueType::arrayValue)) {}

void IndividualEventStatistics::record(const std::string& source, unsigned int send_time,
                                       Event* event) {
    Json::Value entry;
    entry["source"] = source;
    entry["dest"] = event->receiverName();
    entry["send_time"] = send_time;
    entry["receive_time"] = event->timestamp();
    (*root_)[root_->size()] = entry;
}

std::ostream& IndividualEventStatistics::printStats(std::ostream& stream) const {
    return stream << *root_;
}

} // namespace warped