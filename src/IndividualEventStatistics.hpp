#ifndef INDIVIDUAL_EVENT_STATISTICS_HPP
#define INDIVIDUAL_EVENT_STATISTICS_HPP

#include <iosfwd>
#include <memory>
#include <string>
#include <vector>

#include "EventStatistics.hpp"

namespace Json { class Value; }

namespace warped {

class Event;

class IndividualEventStatistics : public EventStatistics {
public:
    IndividualEventStatistics(std::string filename);
    void record(const std::string& source, unsigned int send_time, Event* event);

private:
    std::ostream& printStats(std::ostream& stream) const;
    std::unique_ptr<Json::Value> root_;
};

} // namespace warped

#endif
