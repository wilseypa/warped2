#ifndef SPLAY_TREE_HPP
#define SPLAY_TREE_HPP

/* Splay Tree Library */

/* Include section */
#include <iostream>
#include "Event.hpp"

namespace warped {

/* Splay Tree class */
class SplayTree {
public:
    SplayTree();
    std::shared_ptr<Event> begin();
    bool erase(std::shared_ptr<Event> event);
    void insert(std::shared_ptr<Event> event);

private:

    struct Node {
        std::shared_ptr<Event> data_;
        Node *left_   = nullptr;
        Node *right_  = nullptr;
        Node *parent_ = nullptr;
    };

    void splay( Node *node );
    Node *rotateLeft( Node *node );
    Node *rotateRight( Node *node );

    Node *root_             = nullptr;
    Node *current_          = nullptr;
    unsigned int num_nodes_ = 0;
};

} // namespace warped

#endif /* SPLAY_TREE_HPP */

