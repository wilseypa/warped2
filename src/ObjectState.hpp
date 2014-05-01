#ifndef OBJECT_STATE_H
#define OBJECT_STATE_H

#include <memory>

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

#define Define_ObjectState_Subclass(Type) class Type : public ObjectStateMixin<Type>

#endif
