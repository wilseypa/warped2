#ifndef LOCK_FREE_STACK_HPP
#define LOCK_FREE_STACK_HPP

/** Lock Free Stack **/

/* Include section */
#include <iostream>
#include <atomic>
#include <memory>
#include "Event.hpp"

class LockFreeStack {
public:

    LockFreeStack() = default;

    /* Push new value into the stack */
    void push( std::shared_ptr<warped::Event> e ) {

        if (!e) return;
        auto node   = new Node();
        node->data_ = e;
        do {
            node->next_ = head_.load();
        } while (!std::atomic_compare_exchange_weak(&head_, &(node->next_), node));
    }

    /* Pop a value from the stack */
    std::shared_ptr<warped::Event> pop() {

        Node *node = nullptr;
        do {
            node = head_.load();
            if (!node) return nullptr;
        } while (!std::atomic_compare_exchange_weak(&head_, &node, node->next_));

        auto e = node->data_;
        delete node;
        return e;
    }

private:

    struct Node {
        std::shared_ptr<warped::Event> data_;
        Node *next_ = nullptr;
    };

    std::atomic<Node*> head_  = ATOMIC_VAR_INIT(nullptr);
};

#endif /* LOCK_FREE_STACK_HPP */

