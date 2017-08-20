#ifndef CIRCULAR_LIST_HPP
#define CIRCULAR_LIST_HPP

/** Circular List **/

/* Include section */
#include <iostream>
#include <cassert>

template <class T>
class CircularList {
public:

    /* Create a circular list */
    CircularList() {

        auto new_node = new ListNode();
        head_         = new_node;
        head_->next_  = head_;
        head_->prev_  = head_;
        tail_         = nullptr; /* Tail will be null when list is empty */
        capacity_++;
    }

    /* Check if circular list is empty */
    bool empty() { return (!size_); }

    /* Query the size of valid data inside the circular list */
    size_t size() { return size_; }

    /* Insert a new value at the tail end of the circular list */
    void insert( T value ) {

        /* If Circular List is empty */
        if (!size_) {
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
        if (!size_) tail_ = nullptr;

        return value;
    }

    /* Pop a value from the tail of the circular list */
    T pop_back() {

        assert(size_);
        auto value = tail_->data_;
        tail_ = tail_->prev_;
        size_--;
        if (!size_) tail_ = nullptr;

        return value;
    }

    /* Reads the value from head */
    T read_front() {

        assert(size_);
        return ( head_->data_ );
    }

    /* Reads the value next to head */
    T read_second() {

        assert(size_ >= 2);
        return ( head_->next_->data_ );
    }

    /* Reads the value from tail */
    T read_back() {

        assert(size_);
        return ( tail_->data_ );
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

