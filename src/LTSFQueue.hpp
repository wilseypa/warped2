#ifndef LTSF_QUEUE_HPP
#define LTSF_QUEUE_HPP

#include <memory>
#include <vector>

namespace warped {

class Event;

class LTSFQueue {
public:
    virtual ~LTSFQueue() {}
    virtual bool empty() const = 0;
    virtual unsigned int size() const = 0;
    virtual Event* peek() const = 0;
    virtual std::unique_ptr<Event> pop() = 0;
    virtual void push(std::unique_ptr<Event> event) = 0;
    virtual void push(std::vector<std::unique_ptr<Event>>& events) = 0;
    virtual void push(std::vector<std::unique_ptr<Event>>&& events) = 0;
};

} // namespace warped

#endif