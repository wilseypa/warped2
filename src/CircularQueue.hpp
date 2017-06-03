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
        tail_         = nullptr; /* Tail will be null when list is empty */

        for (unsigned int i = 1; i < length; i++) {
            auto prev_node   = new_node;
            new_node         = new Node();
            prev_node->next_ = new_node;
            new_node->prev_  = prev_node;
        }
        head_->prev_    = new_node;
        new_node->next_ = head_;
        capacity_ = length;
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

        } else { /* Else release the free node after tail from the queue chain */
            node                = tail_->next_;
            tail_->next_        = node->next_;
            node->next_->prev_  = tail_;
        }
        node->data_ = e;

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
            /* Index starts from 1 since comparison starts from second node */
            unsigned int index = 1;

            /* While new event is larger than the compared event */
            while (less(comp->data_, e)) {
                /* If next hop is tail or exceeds the queue size */
                unsigned int next_index = index + HOP_DISTANCE;
                if (next_index >= size_-1) {
                    comp = tail_;
                    index = size_-1;
                    break;
                }
                /* Hop to the next comparison marker */
                while (index < next_index) {
                    comp = comp->next_;
                    index++;
                }
            }

            /* If insert position is just after head */
            if (index == 1) {
                node->prev_  = head_;
                node->next_  = comp;
                head_->next_ = node;
                comp->prev_  = node;

            } else { /* Search backwards for insert position from last comparison marker */
                comp = comp->prev_;
                while (less(e, comp->data_)) {
                    comp = comp->prev_;
                }
                node->prev_         = comp;
                node->next_         = comp->next_;
                comp->next_->prev_  = node;
                comp->next_         = node;
            }
        }
        size_++; /* Increment the queue size */
    }

    /* Deactivate an event from the circular queue */
    bool deactivate( std::shared_ptr<Event> e ) {

        if (!size_) return false;

        /* If node to be deleted is tail */
        if (tail_->data == e) {

            /* If size = 1, then empty the queue.
               Else make the previous element tail */
            tail_ = (size == 1) ? nullptr : tail_->prev_;

            size_--;
            return true;
        }

        /* If node to be deleted is head */
        /* Queue size > 1 based on previous check */
        if (head_->data == e) {

            auto node = head_;
            head_ = head_->next_;

            /* Delete node from head */
            node->prev_->next_  = head_;
            head_->prev_        = node->prev_;

            /* Add deleted node after tail */
            tail_->next_->prev_ = node;
            node->next_         = tail_->next_;
            tail_->next_        = node;
            node->prev_         = tail_;

            size_--;
            return true;
        }

        /* If event to be deleted is less that or 
           greater than events in the circular queue.
           Queues of size <= 2 already checked.        */
        if ( (size_ <= 2) || (*e < *head_->data) || (*tail_->data < *e) ) {
            return false;
        }

        /* Delete event from internal nodes of the circular queue */
        /* Index starts from 2 since comparison starts from third node */
        auto comp  = head_->next_->next_;
        unsigned int index = 2;

        /* While event e is larger than the compared event */
        while (*comp->data_ < *e) {
            /* If next hop is tail or exceeds the queue size */
            unsigned int next_index = index + HOP_DISTANCE;
            if (next_index >= size_-2) {
                comp = tail_->prev_;
                break;
            }
            /* Hop to the next comparison marker */
            while (index < next_index) {
                comp = comp->next_;
                index++;
            }
        }

        /* Search backwards for the event */
        while (e != comp->data_) {
            comp = comp->prev_;
            if (comp == head_ || *comp->data < *e) return false;
        }

        /* Node found, delete it and attach it after tail */
        comp->prev_->next_  = comp->next_;
        comp->next_->prev_  = comp->prev_;

        tail_->next_->prev_ = comp;
        comp->next_         = tail_->next_;
        tail_->next_        = comp;
        comp->prev_         = tail_;

        size_--;
        return true;
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

