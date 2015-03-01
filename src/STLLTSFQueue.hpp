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
    std::shared_ptr<Event> peek() const;
    std::shared_ptr<Event> pop();
    void push(std::shared_ptr<Event> event);
    void push(std::vector<std::shared_ptr<Event>>&& events);

private:
    // use a std::function with a lambda to do event comparison
    typedef std::function<bool (std::shared_ptr<Event>, std::shared_ptr<Event>)> comptype;
    std::priority_queue<std::shared_ptr<Event>, std::vector<std::shared_ptr<Event>>, comptype>
        queue_;
};

} // namespace warped

#endif
