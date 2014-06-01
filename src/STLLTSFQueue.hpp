#ifndef STLLTSF_QUEUE_HPP
#define STLLTSF_QUEUE_HPP

#include <cstddef>
#include <functional>
#include <memory>
#include <queue>
#include <vector>

#include "LTSFQueue.hpp"

namespace warped {

class Event;

// An implementation of an LTSF queue that stores events in a std::priority_queue.
class STLLTSFQueue : public LTSFQueue {
public:
    STLLTSFQueue();
    bool empty() const;
    std::size_t size() const;
    Event* peek() const;
    std::unique_ptr<Event> pop();
    void push(std::unique_ptr<Event> event);
    void push(std::vector<std::unique_ptr<Event>>&& events);

private:
    // use a std::function with a lambda to do event comparison
    typedef std::function<bool (Event*, Event*)> comptype;
    std::priority_queue<Event*, std::vector<Event*>, comptype> queue_;
};

} // namespace warped

#endif