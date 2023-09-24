#ifndef LP_STATE_HPP
#define LP_STATE_HPP

#include <memory>

namespace warped {

// Any LogicalProcess state that is mutable once the simulation begins must
// be stored in an LPState subclass. In most cases subclasses should be
// defined with the WARPED_DEFINE_LP_STATE_STRUCT macro instead of
// manually defining the necessary methods.
struct LPState {
    virtual ~LPState() {}
    virtual std::unique_ptr<LPState> clone() const = 0;
    virtual void restoreState(LPState& os) = 0;
};

// This mixin uses CRTP to define the clone method for any copy-constructable
// subclass.
template<class Derived>
struct LPStateMixin : LPState {
    virtual std::unique_ptr<LPState> clone() const {
        return std::unique_ptr<LPState>(new Derived(static_cast<Derived const&>(*this)));
    }
    virtual void restoreState(LPState& os) {
        static_cast<Derived&>(*this) = std::move(static_cast<Derived&>(os));
    }
};

// If the LPState subclass is copy constructable, this macro will define
// the necessary virtual methods.
//
// An example of using this macro to create a struct with the name MyLPState:
//
//    WARPED_DEFINE_LP_STATE_STRUCT(MyLPState) {
//        int var1;
//        std::String var2;
//    };
#define WARPED_DEFINE_LP_STATE_STRUCT(Type) struct Type : warped::LPStateMixin<Type>

} // namespace warped

#endif

