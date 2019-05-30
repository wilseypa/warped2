#ifndef OUTPUT_MANAGER_HPP
#define OUTPUT_MANAGER_HPP

#include <vector>
#include <deque>

#include "Event.hpp"
#include "TimeWarpEventDispatcher.hpp"

/* This class contains output queues for each local lp and serves as a base class for
 * a specific cancellation technique.
 */

namespace warped {

class TimeWarpOutputManager {
public:
    TimeWarpOutputManager() = default;

    virtual ~TimeWarpOutputManager() = default;

    // Creates an output queue for each lp as well as locks for each output queue.
    void initialize(unsigned int num_local_lps);

    // Insert an event into the output queue for the specified lp
    void insertEvent(std::shared_ptr<Event> input_event, std::shared_ptr<Event> output_event,
        unsigned int local_lp_id);

    // Remove any events from the output queue before the gvt for the specified lp
    unsigned int fossilCollect(unsigned int gvt, unsigned int local_lp_id);

    // Number of events in the output queue for the specified lp.
    std::size_t size(unsigned int local_lp_id);

    // The rollback method will return a vector of negative events that must be sent
    // as anti-messages
    virtual std::unique_ptr<std::vector<std::shared_ptr<Event>>>
        rollback(std::shared_ptr<Event> straggler_event, unsigned int local_lp_id) = 0;

protected:

    struct OutputEvent {
        OutputEvent(std::shared_ptr<Event> ie, std::shared_ptr<Event> oe) :
            input_event_(ie), output_event_(oe) {}

        std::shared_ptr<Event> input_event_;
        std::shared_ptr<Event> output_event_;
    };

    std::unique_ptr<std::vector<std::shared_ptr<Event>>>
        removeEventsSentAfter(std::shared_ptr<Event> straggler_event, unsigned int local_lp_id);

    // Array of local output queues and locks
    std::unique_ptr<std::deque<OutputEvent> []> output_queue_;

    unsigned int num_local_lps_;
};

}

#endif
