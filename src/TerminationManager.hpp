#ifndef TERMINATION_MANAGER_HPP
#define TERMINATION_MANAGER_HPP

#include "serialization.hpp"

namespace warped {

enum class TerminationState { ACTIVE, PASSIVE };

struct TerminationControlMessage {

    WARPED_REGISTER_SERIALIZABLE_MEMBERS(state_indicator)

    TerminationState state_indicator;
};

class TerminationManager {
public:

private:

};

} // warped namespace

#endif
