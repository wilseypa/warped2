#ifndef CIRCULAR_QUEUE_HPP
#define CIRCULAR_QUEUE_HPP

/** Circular Queue **/

/* Include section */
#include <iostream>
#include <cassert>
#include "Event.hpp"

#define HOP_DISTANCE 4

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
    bool empty() { return (!size_); }

    /* Query the event count inside the circular queue */
    size_t size() { return size_; }

    /* Insert an event into the circular queue */
    void insert( std::shared_ptr<Event> e ) {

        /* If Circular Queue is empty */
        if (!size_) {
            tail_ = head_;
            tail_->data_ = e;
            size_++;
            return;
        }

        Node *node = nullptr;
        /* If Circular Queue has no empty spaces left */
        /* Create a new node */
        if (size_ == capacity_) {
            node = new Node();
            capacity_++;

        } else { /* Else release last free node from the queue chain */
            node                = head_->prev_;
            head_->prev_        = node->prev_;
            node->prev_->next_  = head_;
        }
        node->data_ = e;
        size_++;

        /* Compare and insert node at the right place */
        warped::compareEvents less;

        /* If insert before head */
        if (less(e, head_->data_)) {
            node->prev_         = head_->prev_;
            node->next_         = head_;
            head_->prev_->next_ = node;
            head_->prev_        = node;
            head_               = node;

        } else if (less(tail_->data_, e)) { /* Else if insert after tail */
            node->next_         = tail_->next_;
            node->prev_         = tail_;
            tail_->next_->prev_ = node;
            tail_->next_        = node;
            tail_               = node;

        } else { /* Else insert inside the queue */
            auto comp  = head_->next_;
            for (unsigned int index = 1; index < size_; index += HOP_DISTANCE) {

                /* If new event is larger than the compared event */
                if (less(comp->data_, e)) {

                    /* If next node is tail */
                    if (index == size_-1) break;

                    if (index + HOP_DISTANCE >= size_) {
                        comp = tail_;
                        index = size_- HOP_DISTANCE - 1;
                    } else {
                        for (unsigned int h_index = 0; h_index < HOP_DISTANCE; h_index++) {
                            comp = comp->next_;
                        }
                    }
                    continue;
                }

                /* If previous node is head which has already been checked */
                if (index == 1) {
                    node->prev_     = head_;
                    node->next_     = comp;
                    head_->next_    = node;
                    comp->prev_     = node;
                    break;
                }

                /* If any previous hopped node contains that event */
                for (unsigned int h_index = 1; h_index < HOP_DISTANCE; h_index++) {
                    comp = comp->prev_;
                    if (less(comp->data_, e)) {
                        node->prev_         = comp;
                        node->next_         = comp->next_;
                        comp->next_->prev_  = node;
                        comp->next_         = node;
                        break;
                    }
                }
                /* If ideal place is just before previous hop marker */
                node->prev_         = comp->prev_;
                node->next_         = comp;
                comp->prev_->next_  = node;
                comp->prev_         = node;

                break;
            }
        }
    }

    /* Deactivate an event from the circular queue */
    bool erase( std::shared_ptr<Event> e ) {

        if (!size_) return false;

        /* If head needs erasing */
        bool status = false;
        if (head_->data_ == e) {
            head_ = head_->next_;
            size_--;
            if (!size_) tail_ = nullptr;
            status = true;

        } else if (tail_->data_ == e) { /* Else if tail needs erasing */
            tail_ = tail_->prev_;
            size_--;
            status = true;

        } else { /* Else a middle node needs erasing */
            auto node = head_->next_;
            for (unsigned int index = 1; index < size_-1; index += HOP_DISTANCE) {

                /* If node contains a smaller event */
                if (*node->data_ < *e) {

                    /* If next node is tail */
                    if (index == size_-2) break;

                    /* If it is the last iteration, check for remaining nodes before tail */
                    if (index + HOP_DISTANCE >= size_-1) {
                        node = tail_->prev_;
                        index = size_- HOP_DISTANCE - 2;

                    } else {
                        for (unsigned int h_index = 0; h_index < HOP_DISTANCE; h_index++) {
                            node = node->next_;
                        }
                    }
                    continue;
                }

                /* If node contains that event */
                if (node->data_ == e) {
                    node->prev_->next_  = node->next_;
                    node->next_->prev_  = node->prev_;
                    node->next_         = head_;
                    node->prev_         = head_->prev_;
                    head_->prev_->next_ = node;
                    head_->prev_        = node;
                    size_--;
                    status = true;
                    break;
                }

                /* If previous node is head which has already been checked */
                if (index == 1) break;

                /* If any previous hopped node contains that event */
                for (unsigned int h_index = 1; h_index < HOP_DISTANCE; h_index++) {
                    node = node->prev_;
                    if (node->data_ == e) {
                        node->prev_->next_  = node->next_;
                        node->next_->prev_  = node->prev_;
                        node->next_         = head_;
                        node->prev_         = head_->prev_;
                        head_->prev_->next_ = node;
                        head_->prev_        = node;
                        size_--;
                        status = true;
                        break;
                    }
                }
                break;
            }
        }
        return status;
    }

    /* Pop an event from the head of the circular queue */
    std::shared_ptr<Event> pop_front() {

        if (!size_) return nullptr;

        auto e = head_->data_;
        head_ = head_->next_;
        size_--;
        if (!size_) tail_ = nullptr;
        return e;
    }

    /* Reads the event from the head of the circular queue */
    std::shared_ptr<Event> read_front() {

        return (size_ ? head_->data_ : nullptr);
    }

private:

    /* Node : Structure of each node */
    struct Node {

        std::shared_ptr<Event> data_;
        Node *prev_ = nullptr;
        Node *next_ = nullptr;
    };

    Node *head_         = nullptr;  /* Marks the beginning of the circular queue in use */
    Node *tail_         = nullptr;  /* Marks the end of the circular queue in use */
    size_t size_        = 0;        /* Size of the circular queue currently in use */
    size_t capacity_    = 0;        /* Capacity of the circular queue */ 
};

} // namespace warped

#endif /* CIRCULAR_QUEUE_HPP */

