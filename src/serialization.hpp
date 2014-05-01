#include "cereal/archives/json.hpp"
#include "cereal/types/polymorphic.hpp"

#define WARPED_REGISTER_SERIALIZABLE_MEMBERS(...) \
    template<class Archive> \
    void serialize(Archive& ar) { ar(__VA_ARGS__); }

#define WARPED_REGISTER_POLYMORPHIC_SERIALIZABLE_CLASS CEREAL_REGISTER_TYPE
    