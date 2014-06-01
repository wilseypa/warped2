// Since LTSF queues own events that are pushed in, it's safe to store the
// events as raw pointers. 

#include "STLLTSFQueue.hpp"

#include <cstddef>
#include <memory>
#include <queue>
#include <vector>

#include "Event.hpp"

namespace warped {

STLLTSFQueue::STLLTSFQueue()
    : queue_([](Event* lhs, Event* rhs) {
                    return lhs->timestamp() > rhs->timestamp();
                }) {}

bool STLLTSFQueue::empty() const {
    return queue_.empty();
}

std::size_t STLLTSFQueue::size() const {
    return queue_.size();
}

Event* STLLTSFQueue::peek() const {
    if (queue_.empty()) {
        return nullptr;
    }
    return queue_.top();
}

std::unique_ptr<Event> STLLTSFQueue::pop() {
    auto ret = queue_.top();
    queue_.pop();
    return std::unique_ptr<Event>{ret};
}

void STLLTSFQueue::push(std::unique_ptr<Event> event) {
    queue_.push(event.release());
}

void STLLTSFQueue::push(std::vector<std::unique_ptr<Event>>&& events) {
    for (auto& event: events) {
        queue_.push(event.release());
    }
}

} // namespace warped

