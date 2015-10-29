#ifndef SPLAY_TREE_HPP
#define SPLAY_TREE_HPP

/* Splay Tree Library */

/* Include section */
#include <iostream>
#include "Event.hpp"

namespace warped {

/* Node class */
class Node {
public:
    Node(std::shared_ptr<Event> event):
        left_node_(nullptr), right_node_(nullptr),
        parent_node_(nullptr), data_(event) {}

    std::shared_ptr<Node> getLeftNode() {
        return left_node_;
    }

    std::shared_ptr<Node> getRightNode() {
        return right_node_;
    }

    std::shared_ptr<Node> getParentNode() {
        return parent_node_;
    }

    std::shared_ptr<Event> getData() {
        return data_;
    }

    void setLeftNode(std::shared_ptr<Node> node) {
        left_node_ = node;
    }

    void setRightNode(std::shared_ptr<Node> node) {
        right_node_ = node;
    }

    void setParentNode(std::shared_ptr<Node> node) {
        parent_node_ = node;
    }

private:
    std::shared_ptr<Node>   left_node_;
    std::shared_ptr<Node>   right_node_;
    std::shared_ptr<Node>   parent_node_;
    std::shared_ptr<Event>  data_;
};


/* Splay Tree class */
class SplayTree {
public:
    SplayTree();
    std::shared_ptr<Event> begin();
    bool erase(std::shared_ptr<Event> event);
    void insert(std::shared_ptr<Event> event);

private:
    void splay(std::shared_ptr<Node> node);
    std::shared_ptr<Node> rotateLeft(std::shared_ptr<Node> node);
    std::shared_ptr<Node> rotateRight(std::shared_ptr<Node> node);

    std::shared_ptr<Node> root_;
    std::shared_ptr<Node> current_;
    unsigned int num_nodes_ = 0;
};

} // namespace warped

#endif /* SPLAY_TREE_HPP */

