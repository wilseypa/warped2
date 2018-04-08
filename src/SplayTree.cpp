#include "SplayTree.hpp"
#include <cassert>

namespace warped {

SplayTree::SplayTree() = default;

/* Search lowest event */
std::shared_ptr<Event> SplayTree::begin() {

    // If splay tree is invalid
    if (!root_ || !num_nodes_) {
        root_       = nullptr;
        current_    = nullptr;
        num_nodes_  = 0;

        return nullptr;
    }

    if (!current_) {
        for( current_ = root_ ; current_->left_ ; current_ = current_->left_ );
    }
    return current_->data_;
}

/* Erase event */
bool SplayTree::erase( std::shared_ptr<Event> event ) {

    assert(event && root_);

    if (!current_) {
        for( current_ = root_ ; current_->left_ ; current_ = current_->left_ );
        return false;
    }

    // Search for the event to be deleted
    // Also handles the condition when current = root
    auto node = (*event <= *current_->data_) ? current_ : root_;
    while (node) {
        if (*event < *node->data_) {
            node = node->left_;
        } else if (event == node->data_) { // Comparing address is sufficient
            break;
        } else {
            node = node->right_;
        }
    }

    if (!node) { return false; }

    // Remove the node if found
    if (node != root_) {
        splay(node);
        if (node != root_) assert(0);
    }
    auto left  = root_->left_;
    auto right = root_->right_;
    num_nodes_--;
    delete root_; // node == root_
    root_      = nullptr;

    // Re-construct the tree
    if (left) {
        root_ = left;
        root_->parent_ = nullptr;
        if (right) {
            while (left->right_) {
                left = left->right_;
            }
            left->right_    = right;
            right->parent_  = left;
        }
    } else {
        root_ = right;
        if (root_) { root_->parent_  = nullptr; }
    }

    // Re-calculate the current
    if (root_) {
        for( current_ = root_ ; current_->left_ ; current_ = current_->left_ );
    }
    return true;
}

/* Insert event */
void SplayTree::insert( std::shared_ptr<Event> event ) {

    assert(event);
    auto new_node = new SplayTree::Node();
    assert(new_node);
    new_node->data_ = event;
    num_nodes_++;

    if (!root_) {
        root_    = new_node;
        current_ = new_node;
        return;
    }

    assert(current_);
    if (!current_->data_) {
        for( current_ = root_ ; current_->left_ ; current_ = current_->left_ );
    }

    if (*event <= *current_->data_) {
        current_->left_   = new_node;
        new_node->parent_ = current_;
        current_ = new_node;
        splay(new_node);
        return;
    }

    auto node = root_;
    while (1) {
        if (*event <= *node->data_) {
            if (node->left_) {
                node = node->left_;
            } else {
                node->left_ = new_node;
                break;
            }
        } else {
            if (node->right_) {
                node = node->right_;
            } else {
                node->right_ = new_node;
                break;
            }
        }
    }
    new_node->parent_ = node;
    splay(new_node);
}

/* Splay */
void SplayTree::splay( SplayTree::Node *node ) {

    unsigned int splay_cnt  = 0;
    unsigned int max_splay  = num_nodes_ / 2;
    bool parent_flag        = false;
    bool grandpa_flag = false;

    while (node != root_) {
        auto parent = node->parent_;
        if (parent == root_) {
            if (parent->left_ == node) {
                rotateRight(parent);
            } else {
                rotateLeft(parent);
            }
            break;
        } else {
            parent_flag = (parent->left_ == node) ? false : true;
            auto grand_parent = parent->parent_;
            grandpa_flag = (grand_parent->left_ == parent) ? false : true;
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
SplayTree::Node *SplayTree::rotateLeft( SplayTree::Node *node ) {

    if (!node) return nullptr;

    auto right = node->right_;
    if (!right) { return node; }

    auto left       = right->left_;
    auto parent     = node->parent_;
    node->parent_   = right;
    right->left_    = node;
    node->right_    = left;

    if (left) { left->parent_ = node; }
    if (node == root_) {
        root_ = right;
        root_->parent_ = nullptr;

    } else {
        right->parent_ = parent;
        if (parent->left_ == node) {
            parent->left_ = right;
        } else {
            parent->right_ = right;
        }
    }
    return right;
}

/* Rotate Right */
SplayTree::Node *SplayTree::rotateRight( SplayTree::Node *node ) {

    if (!node) return nullptr;

    auto left = node->left_;
    if (!left) { return node; }

    auto right      = left->right_;
    auto parent     = node->parent_;
    node->parent_   = left;
    left->right_    = node;
    node->left_     = right;

    if (right) { right->parent_ = node; }
    if (node == root_) {
        root_ = left;
        root_->parent_ = nullptr;

    } else {
        left->parent_ = parent;
        if (parent->left_ == node) {
            parent->left_ = left;
        } else {
            parent->right_ = left;
        }
    }
    return left;
}

} // namespace warped
