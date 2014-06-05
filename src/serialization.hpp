#ifndef WARPED_SERIALIZATION_HPP
#define WARPED_SERIALIZATION_HPP

// Macros used in Event serialization.

#include "cereal/archives/binary.hpp"
#include "cereal/types/polymorphic.hpp"

// Every serializable class must pass all class members to this macro inside
// the class definition. Since unserialiazed values are copied into the class
// members, the members cannot be const.
#define WARPED_REGISTER_SERIALIZABLE_MEMBERS(...) \
    template<class Archive> \
    void serialize(Archive& ar) { ar(__VA_ARGS__); }

// Every polymorphic subclass must call this macro with the class name outside
// of the class definition. Unfortunately, if this is used in a header that is
// included more than once (even if its surrounded by an include guard), the
// code will fail to compile. Use this in a cpp file instead.
#define WARPED_REGISTER_POLYMORPHIC_SERIALIZABLE_CLASS CEREAL_REGISTER_TYPE
    
#endif
