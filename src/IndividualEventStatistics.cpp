#include "IndividualEventStatistics.hpp"

#include <iostream>
#include <memory>
#include <string>
#include <utility>

#include "json/json.h"

#include "Event.hpp"
#include "EventStatistics.hpp"

namespace {
// Return a string properly escaped for csv. If the input contains \r or \n,
// the result will be incorrect.
std::string csvEscape(std::string in) {
    if (in.find_first_of(",\"") == std::string::npos) {
        return in;
    }

    std::string out = "\"";
    for (const auto& c : in) {
        if (c == '"') {
            out += "\"\"";
        } else {
            out += c;
        }
    }
    out += "\"";
    return out;
}
} // namespace

namespace warped {

IndividualEventStatistics::IndividualEventStatistics(std::string filename, OutputType output_type)
    : EventStatistics(std::move(filename)), output_type_(output_type), entries_() {}

void IndividualEventStatistics::record(const std::string& source, unsigned int send_time,
                                       Event* event) {
    entries_.emplace_back(  source,
                            event->receiverName(),
                            send_time,
                            event->timestamp(),
                            event->size() + event->base_size()  );
}
void IndividualEventStatistics::record(const std::string& source, unsigned int send_time,
                             const std::vector<std::shared_ptr<Event>>& events) {
    for (const auto& e : events) {
        record(source, send_time, e.get());
    }
}


std::ostream& IndividualEventStatistics::printStats(std::ostream& stream) const {
    if (output_type_ == OutputType::Json) {
        return printJson(stream);
    }
    return printCsv(stream);
}

std::ostream& IndividualEventStatistics::printJson(std::ostream& stream) const {
    Json::Value root {Json::ValueType::arrayValue};
    root.resize(entries_.size());

    for (unsigned int i = 0; i < entries_.size(); ++i) {
        root[i]["source"] = entries_[i].source;
        root[i]["dest"] = entries_[i].dest;
        root[i]["send_time"] = entries_[i].send_time;
        root[i]["receive_time"] = entries_[i].receive_time;
        root[i]["event_size"] = entries_[i].event_size;
    }

    return stream << root;
}

std::ostream& IndividualEventStatistics::printCsv(std::ostream& stream) const {
    stream << "# Source,Destination,Send Time,Receive Time,Event Size\n";
    for (const auto& entry : entries_) {
        stream << csvEscape(entry.source) << ','
               << csvEscape(entry.dest) << ','
               << std::to_string(entry.send_time) << ','
               << std::to_string(entry.receive_time) << ','
               << std::to_string(entry.event_size) << '\n';
    }

    return stream;
}

} // namespace warped
