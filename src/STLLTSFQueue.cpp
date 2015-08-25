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
    : queue_([](std::shared_ptr<Event> lhs, std::shared_ptr<Event> rhs) {
                    return !(*lhs <= *rhs);
                }) {}

bool STLLTSFQueue::empty() const {
    return queue_.empty();
}

std::size_t STLLTSFQueue::size() const {
    return queue_.size();
}

std::shared_ptr<Event> STLLTSFQueue::peek() const {
    if (queue_.empty()) {
        return nullptr;
    }
    return queue_.top();
}

std::shared_ptr<Event> STLLTSFQueue::pop() {
    if (queue_.empty()) {
        return nullptr;
    }
    auto ret = queue_.top();
    queue_.pop();
    return std::shared_ptr<Event>{ret};
}

void STLLTSFQueue::push(std::shared_ptr<Event> event) {
    queue_.push(event);
}

void STLLTSFQueue::push(std::vector<std::shared_ptr<Event>>&& events) {
    for (auto& event: events) {
        queue_.push(event);
    }
}

} // namespace warped

