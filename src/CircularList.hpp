#ifndef CIRCULAR_LIST_HPP
#define CIRCULAR_LIST_HPP

/** Circular List **/

/* Include section */
#include <iostream>
#include <cassert>

template <class T>
class CircularList {
public:

    CircularList() = default;

    /* Create a circular list of specified length */
    CircularList( unsigned int length ) {
        assert(length); /* Length can't be 0 */
        auto new_node = new ListNode();
        head_         = new_node;
        head_->next_  = head_;
        head_->prev_  = head_;
        tail_         = nullptr; /* Tail will be null when list is empty */
        capacity_++;
        for (unsigned int i = 1; i < length; i++) {
            auto prev_node   = new_node;
            new_node         = new ListNode();
            prev_node->next_ = new_node;
            new_node->prev_  = prev_node;
            head_->prev_     = new_node;
            new_node->next_  = head_;
            capacity_++;
        }
    }

    /* Check if circular list is empty */
    bool empty() {
        return (!size_);
    }

    /* Query the size of valid data inside the circular list */
    size_t size() {
        return size_;
    }

    /* Insert a new value at the tail end of the circular list */
    void insert( T value ) {
        /* If Circular List is empty */
        if (tail_ == nullptr) {
            tail_ = head_;
        } else {
            /* If Circular List is full - create a new node */
            if (tail_->next_ == head_) {
                auto new_node   = new ListNode();
                tail_->next_    = new_node;
                new_node->next_ = head_;
                new_node->prev_ = tail_;
                head_->prev_    = new_node;
                capacity_++;
            }
            tail_ = tail_->next_;
        }
        tail_->data_ = value;
        size_++;
    }

    /* Pop a value from the head of the circular list */
    T pop_front() {
        assert(size_);
        auto value = head_->data_;
        head_ = head_->next_;
        size_--;
        if (!size_) {
            tail_ = nullptr;
        }
        return value;
    }

    /* Pop a value from the tail of the circular list */
    T pop_back() {
        assert(size_);
        auto value = tail_->data_;
        tail_ = tail_->prev_;
        size_--;
        if (!size_) {
            tail_ = nullptr;
        }
        return value;
    }

    /* Reads the value from head */
    T read_head() {
        assert(size_);
        auto value = head_->data_;
        return value;
    }

    /* Reads the value next to head */
    T read_next2head() {
        assert(size_ >= 2);
        auto value = head_->next_->data_;
        return value;
    }

    /* Reads the value from tail */
    T read_tail() {
        assert(size_);
        auto value = tail_->data_;
        return value;
    }

private:

    /* List Node : Structure of each node */
    struct ListNode {
        T data_;
        ListNode *prev_ = nullptr;
        ListNode *next_ = nullptr;
    };

    ListNode *head_  = nullptr;
    ListNode *tail_  = nullptr;
    size_t size_     = 0; /* Size of the circular list currently in use */
    size_t capacity_ = 0; /* Capacity of the circular list */ 
};


#endif /* CIRCULAR_LIST_HPP */

