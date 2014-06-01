#ifndef EVENT_STATISTICS_HPP
#define EVENT_STATISTICS_HPP

#include <iosfwd>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace warped {

class Event;

class EventStatistics {
public:
    EventStatistics(std::string filename) : filename_(std::move(filename)) {}

    // Record an event.
    virtual void record(const std::string& source, unsigned int send_time,
                        Event* event) = 0;
    
    // Record zero or more events at once
    virtual void record(const std::string& source, unsigned int send_time,
                        const std::vector<std::unique_ptr<Event>>& events);

    // Write the statistics to a file
    virtual void writeToFile();

    // Write the statistics to a stream
    friend std::ostream& operator<<(std::ostream& stream, const EventStatistics& stats);

protected:
    // The name of the file that that the stats will be printed to.
    std::string filename_;
private:
    // Implementation of operator<<
    virtual std::ostream& printStats(std::ostream& stream) const = 0;
};

} // namespace warped

#endif
