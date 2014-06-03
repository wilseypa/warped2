#ifndef OBJECT_STATE_HPP
#define OBJECT_STATE_HPP

#include <memory>

namespace warped {

// Any SimulationObject state that is mutable once the simulation begins must
// be stored in an ObjectState subclass. In most cases subclasses should be
// defined with the WARPED_DEFINE_OBJECT_STATE_STRUCT macro instead of
// manually defining the necessary methods.
struct ObjectState {
    virtual ~ObjectState() {}
    virtual std::unique_ptr<ObjectState> clone() const = 0;

};

// This mixin uses CRTP to define the clone method for any copy-constructable
// subclass.
template<class Derived>
struct ObjectStateMixin : ObjectState {
    virtual std::unique_ptr<ObjectState> clone() const {
        return std::unique_ptr<ObjectState>(new Derived(static_cast<Derived const&>(*this)));
    }
};

// If the ObjectState subclass is copy constructable, this macro will define
// the necessary virtual methods.
//
// An example of using this macro to create a struct with the name MyObjectState:
//
//    WARPED_DEFINE_OBJECT_STATE_STRUCT(MyObjectState) {
//        int var1;
//        std::String var2;
//    };
#define WARPED_DEFINE_OBJECT_STATE_STRUCT(Type) struct Type : warped::ObjectStateMixin<Type>

} // namespace warped

#endif

