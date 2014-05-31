#include <iosfwd>
#include <string>

#include "Event.hpp"
#include "EventStatistics.hpp"
#include "utility/warnings.hpp"

namespace warped {

// An EventStatistics class that performs no actions
class NullEventStatistics : public EventStatistics {
public:
    void record(const std::string& source, unsigned int send_time,
                Event* event) {
        unused(source, send_time, event);
    }

    void writeToFile(const std::string& filename) { unused(filename); }
private:
    std::ostream& printStats(std::ostream& stream) const { return stream; };
};

} // namespace warped
