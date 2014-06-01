#ifndef LTSF_QUEUE_HPP
#define LTSF_QUEUE_HPP

#include <cstddef>
#include <memory>
#include <vector>

namespace warped {

class Event;

// A Least Time Stamp First (LTSF) Queue interface
//
// The interface for Warped LTSF queues differs slightly from STL queues. LTSF
// Queues own the events they store, and so must give up ownership of the
// events when pop() is called instead of simply deleting the objects like STL
// queues do.
class LTSFQueue {
public:
    virtual ~LTSFQueue() {}

    // Return true if the queue is empty.
    virtual bool empty() const = 0;

    // Return the size of the queue.
    virtual std::size_t size() const = 0;

    // Return a non-owning pointer to the top of the queue, or nullptr iff the
    // queue is empty.
    virtual Event* peek() const = 0;

    // Remove the lowest timestamped event from the queue and give ownership
    // to the caller. It is undefined behavior to call this function when
    // size() == 0.
    virtual std::unique_ptr<Event> pop() = 0;

    // Add one event to the queue. Ownership of the event is transferred to
    // the queue.
    virtual void push(std::unique_ptr<Event> event) = 0;

    // Add multiple events to the queue at once. Ownership of all events is
    // transfered to the queue.
    virtual void push(std::vector<std::unique_ptr<Event>>&& events) = 0;
};

} // namespace warped

#endif