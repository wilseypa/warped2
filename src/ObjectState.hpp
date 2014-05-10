#ifndef OBJECT_STATE_H
#define OBJECT_STATE_H

#include <memory>

namespace warped {

class ObjectState {
public:
    virtual ~ObjectState() {}
    virtual std::unique_ptr<ObjectState> clone() const = 0;

};

template<class Derived>
class ObjectStateMixin : public ObjectState {
public:
    virtual std::unique_ptr<ObjectState> clone() const {
        return std::unique_ptr<ObjectState>(new Derived(static_cast<Derived const&>(*this)));
    }
};

#define WARPED_DEFINE_OBJECT_STATE_SUBCLASS(Type) class Type : public warped::ObjectStateMixin<Type>

} // namespace warped

#endif

