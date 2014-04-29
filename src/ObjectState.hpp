#ifndef OBJECT_STATE_H
#define OBJECT_STATE_H

#include <memory>

template<class Derived>
class ObjectState {
public:
    virtual ~ObjectState() {}

    virtual std::unique_ptr<ObjectState> clone() const {
        return std::unique_ptr<ObjectState>(new Derived(static_cast<Derived const&>(*this)));
    }
};

#endif