#ifndef PROFILE_GUIDED_PARTITIONER_HPP
#define PROFILE_GUIDED_PARTITIONER_HPP

#include <iosfwd>
#include <vector>

#include "Partitioner.hpp"

namespace warped {

class SimulationObject;

class ProfileGuidedPartitioner : public Partitioner {
public:
    ProfileGuidedPartitioner(std::istream& input);

    std::vector<std::vector<SimulationObject*>> partition(
             const std::vector<SimulationObject*>& objects,
             const unsigned int num_partitions) const;

private:
    std::istream& input;
};

} // namespace warped


#endif
