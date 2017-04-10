#ifndef CIRCULAR_QUEUE_HPP
#define CIRCULAR_QUEUE_HPP

/** Circular Queue **/

/* Include section */
#include <iostream>
#include <cassert>
#include "Event.hpp"

namespace warped {

class CircularQueue {
public:

    CircularQueue() = default;

    /* Create a circular queue of specified length */
    CircularQueue( unsigned int length ) {
        assert(length); /* Length can't be 0 */
        auto new_node = new Node();
        head_         = new_node;
        head_->next_  = head_;
        head_->prev_  = head_;
        tail_         = nullptr; /* Tail will be null when list is empty */
        capacity_++;
        for (unsigned int i = 1; i < length; i++) {
            auto prev_node   = new_node;
            new_node         = new Node();
            prev_node->next_ = new_node;
            new_node->prev_  = prev_node;
            head_->prev_     = new_node;
            new_node->next_  = head_;
            capacity_++;
        }
    }

    /* Check if circular queue is empty */
    bool empty() {
        return (!size_);
    }

    /* Query the event count inside the circular queue */
    size_t size() {
        return size_;
    }

    /* Insert an event into the circular queue */
    void insert( std::shared_ptr<Event> e ) {

        /* If Circular Queue is empty */
        if (tail_ == nullptr) {
            tail_ = head_;
            tail_->data_ = e;

        } else { /* If Circular Queue has no empty spaces left */
            if (size_ == capacity_) {
                auto new_node       = new Node();
                new_node->next_     = head_;
                new_node->prev_     = head_->prev_;
                head_->prev_->next_ = new_node;
                head_->prev_        = new_node;
                capacity_++;
            }
            head_ = head_->prev_; /* Set as new head */
            head_->data_ = e;

            /* Compare and swap data if needed while sorting */
            auto first  = head_;
            auto second = head_->next_;
            for (unsigned int index = 0; (index < size_) && 
                    !compareEvents(first->data_, second->data_); index++) {
                auto temp_data = first->data_;
                first_->data_  = second_->data_;
                second_->data_ = temp_data;
                first = second;
                second = first->next_;
            }
        }
        size_++;
    }

    /* Erase an event from the circular queue */
    void erase( std::shared_ptr<Event> e ) {
        auto node  = head_;
        for (unsigned int index = 0; index < size_; index++) {
            //TODO
            node = node->next_;
        }
    }

    /* Pop an event from the head of the circular queue */
    std::shared_ptr<Event> pop_front() {
        assert(size_);
        auto e = head_->data_;
        head_ = head_->next_;
        size_--;
        if (!size_) {
            tail_ = nullptr;
        }
        return e;
    }

    /* Reads the event from the head of the circular queue */
    std::shared_ptr<Event> read_front() {
        assert(size_);
        auto e = head_->data_;
        return e;
    }

private:

    /* Node : Structure of each node */
    struct Node {
        std::shared_ptr<Event> data_;
        Node *prev_ = nullptr;
        Node *next_ = nullptr;
    };

    Node *head_  = nullptr;
    Node *tail_  = nullptr;
    size_t size_     = 0; /* Size of the circular queue currently in use */
    size_t capacity_ = 0; /* Capacity of the circular queue */ 
};

} // namespace warped

#endif /* CIRCULAR_QUEUE_HPP */

