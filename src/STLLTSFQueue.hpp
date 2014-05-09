#ifndef STLLTSF_QUEUE_HPP
#define STLLTSF_QUEUE_HPP

#include <functional>
#include <memory>
#include <queue>
#include <vector>

#include "LTSFQueue.hpp"

namespace warped {

class Event;

class STLLTSFQueue : public LTSFQueue {
public:
    STLLTSFQueue();
    bool empty() const;
    unsigned int size() const;
    Event* peek() const;
    std::unique_ptr<Event> pop();
    void push(std::unique_ptr<Event> event);
    void push(std::vector<std::unique_ptr<Event>>& events);
    void push(std::vector<std::unique_ptr<Event>>&& events);

private:
    typedef std::function<bool (Event*, Event*)> comptype;
    std::priority_queue<std::unique_ptr<Event>, std::vector<Event*>, comptype> queue_;
};

} // namespace warped

#endif