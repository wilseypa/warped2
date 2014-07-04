#ifndef INDIVIDUAL_EVENT_STATISTICS_HPP
#define INDIVIDUAL_EVENT_STATISTICS_HPP

#include <iosfwd>
#include <string>
#include <vector>

#include "EventStatistics.hpp"

namespace Json { class Value; }

namespace warped {

class Event;

class IndividualEventStatistics : public EventStatistics {
public:
    enum class OutputType {Json, Csv};

    struct entry {
        entry() = default;
        entry(std::string s, std::string d, unsigned int st, unsigned int rt)
            : source(s), dest(d), send_time(st), receive_time(rt) {}
        std::string source;
        std::string dest;
        unsigned int send_time;
        unsigned int receive_time;
    };

    IndividualEventStatistics(std::string filename, OutputType output_type);
    void record(const std::string& source, unsigned int send_time, Event* event);

private:
    std::ostream& printStats(std::ostream& stream) const;
    std::ostream& printJson(std::ostream& stream) const;
    std::ostream& printCsv(std::ostream& stream) const;

    OutputType output_type_;
    std::vector<entry> entries_;
};

} // namespace warped

#endif
