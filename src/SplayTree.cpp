#include "SplayTree.hpp"
#include <cassert>

namespace warped {

SplayTree::SplayTree():
    root(nullptr), current(nullptr) {}

/* Search lowest event */
std::shared_ptr<Event> SplayTree::begin() {

    std::shared_ptr<Event> event = nullptr;
    if (!root_ || !num_nodes_) {
        root_    = nullptr;
        current_ = nullptr;
        num_nodes_ = 0;
    } else {
        if (!current_) {
            std::shared_ptr<Node> node = root_;
            while (node->getLeftNode()) {
                node = node->getLeftNode();
            }
            current_ = node;
        } else {
            event = current_->getData();
        }
    }
    return event;
}

/* Erase event */
bool SplayTree::erase(std::shared_ptr<Event> event) {

    std::shared_ptr<Node> del_node = nullptr;
    std::shared_ptr<Node> node     = nullptr;
    std::shared_ptr<Node> left     = nullptr;
    std::shared_ptr<Node> right    = nullptr;

    assert(event != nullptr);

    if (!root_) assert(!current_);
    if (!current_) {
        node = root_;
        while (node->getLeftNode()) {
            node = node->getLeftNode();
        }
        current_ = node;
        return;
    }

    // Search for the event to be deleted
    // Also handles the condition when current = root
    if (event <= current_->getData()) {
        node = current_;
    } else {
        node = root_;
    }
    while (node) {
        if (event < node->getData()) {
            node = node->getLeftNode();
        } else if (event == node->getData()) {
            break;
        } else {
            node = node->getRightNode();
        }
    }
    if (!node) {
        return;
    }

    // Remove the node if found
    if (node != root_) {
        splay(node);
        if (node != root_) assert(0);
    }
    left  = root_->getLeftNode();
    right = root_->getRightNode();

    num_nodes_--;

    // Re-construct the tree
    if (left) {
        root_ = left;
        root_->setParentNode(nullptr);
        if (right) {
            while (left->getRightNode()) {
                left = left->getRightNode();
            }
            left->setRightNode(right);
            right->setParentNode(left);
        }
    } else {
        root_ = right;
        if (root_) {
            root_->setParentNode(nullptr);
        }
    }

    // Re-calculate the current
    if (root_) {
        current_ = root_;
        while (current_->getLeftNode()) {
            current_ = current_->getLeftNode();
        }
    }
}

/* Insert event */
void SplayTree::insert(std::shared_ptr<Event> event) {

    std::shared_ptr<Node> node  = nullptr;
    std::shared_ptr<Node> left  = nullptr;
    std::shared_ptr<Node> right = nullptr;
    assert(event);

    std::shared_ptr<Node> new_node = std::make_shared<Node>(event);
    num_nodes_++;

    if (!root_) {
        root_    = new_node;
        current_ = new_node;
        return;
    }

    if (!current_->getData()) {
        node = root_;
        while (node->getLeftNode()) {
            node = node->getLeftNode();
        }
        current_ = node;
        assert(current_->getData());
    }

    if (event <= current_->getData()) {
        current_->setLeftNode(new_node);
        new_node->setParentNode(current_);
        current_ = new_node;
        splay(new_node);
        return;
    }

    node = root_;
    while (1) {
        if (event <= node->getData()) {
            left = node->getLeftNode();
            if (left) {
                node = left;
            } else {
                node->setLeftNode(new_node);
                break;
            }
        } else {
            right = node->getRightNode();
            if (right) {
                node = right;
            } else {
                node->setRightNode(new_node);
                break;
            }
        }
    }
    new_node->setParentNode(node);
    splay(new_node);
}

/* Splay */
void splay(std::shared_ptr<Node> node) {

    std::shared_ptr<Node> parent       = nullptr;
    std::shared_ptr<Node> grand_parent = nullptr;
    unsigned int splay_cnt = 0;
    unsigned int max_splay = num_nodes_ / 2;
    bool parent_flag  = false;
    bool grandpa_flag = false;

    while (node != root_) {
        parent = node->getParentNode();
        if (parent == root_) {
            if (parent->getLeftNode() == node) {
                rotateRight(parent);
            } else {
                rotateLeft(parent);
            }
            break;
        } else {
            parent_flag = (parent->getLeftNode() == node) ? false : true;
            grand_parent = parent->getParentNode();
            grandpa_flag = (grand_parent->getLeftNode() == parent) ? false : true;
            if (parent_flag != grandpa_flag) { //Zig Zag
                if (!parent_flag) {
                    rotateRight(parent);
                    rotateLeft(grand_parent);
                } else {
                    rotateLeft(parent);
                    rotateRight(grand_parent);
                }
            } else { // Zig Zig
                if (!parent_flag) {
                    rotateRight(grand_parent);
                    rotateRight(parent);
                } else {
                    rotateLeft(grand_parent);
                    rotateLeft(parent);
                }
            }
        }

        if (splay_cnt >= max_splay) break;
        splay_cnt++;
    }
}

/* Rotate Left */
std::shared_ptr<Node> rotateLeft(std::shared_ptr<Node> node) {

    std::shared_ptr<Node> right  = nullptr;
    std::shared_ptr<Node> left   = nullptr;
    std::shared_ptr<Node> parent = nullptr;

    if (!node) return nullptr;
    right = node->getRightNode();
    if (!right) {
        return node;
    }
    left = right->getLeftNode();
    parent = node->getParentNode();
    node->setParentNode(right);
    right->setLeftNode(node);
    node->setRightNode(left);
    if (left) {
        left->setParentNode(node);
    }
    if (node == root_) {
        root_ = right;
        root_->setParentNode(nullptr);
    } else {
        right->setParentNode(parent);
        if (parent->getLeftNode() == node) {
            parent->setLeftNode(right);
        } else {
            parent->setRightNode(right);
        }
    }
    return right;
}

/* Rotate Right */
std::shared_ptr<Node> rotateRight(std::shared_ptr<Node> node) {

    std::shared_ptr<Node> right  = nullptr;
    std::shared_ptr<Node> left   = nullptr;
    std::shared_ptr<Node> parent = nullptr;

    if (!node) return nullptr;
    left = node->getLeftNode();
    if (!left) {
        return node;
    }
    right = left->getRightNode();
    parent = node->getParentNode();
    node->setParentNode(left);
    left->setRightNode(node);
    node->setLeftNode(right);
    if (right) {
        right->setParentNode(node);
    }
    if (node == root_) {
        root_ = left;
        root_->setParentNode(nullptr);
    } else {
        left->setParentNode(parent);
        if (parent->getLeftNode() == node) {
            parent->setLeftNode(left);
        } else {
            parent->setRightNode(left);
        }
    }
    return left;
}

} // namespace warped
