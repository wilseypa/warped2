// Macros used in Event serialization.

#include "cereal/archives/binary.hpp"
#include "cereal/types/polymorphic.hpp"

// Every serializable class must pass all class members to this macro inside
// the class definition.
#define WARPED_REGISTER_SERIALIZABLE_MEMBERS(...) \
    template<class Archive> \
    void serialize(Archive& ar) { ar(__VA_ARGS__); }

// Every polymorphic subclass must call this macro with the class name outside
// of the class definition.
#define WARPED_REGISTER_POLYMORPHIC_SERIALIZABLE_CLASS CEREAL_REGISTER_TYPE
    