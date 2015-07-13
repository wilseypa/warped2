#ifndef ROUND_ROBIN_PARTITIONER_HPP
#define ROUND_ROBIN_PARTITIONER_HPP

#include <vector>

#include "Partitioner.hpp"

namespace warped {

class SimulationObject;

class RoundRobinPartitioner : public Partitioner {
public:
    RoundRobinPartitioner(std::vector<float> part_weights = {}) : part_weights_(part_weights) {}

    std::vector<std::vector<SimulationObject*>> partition(
             const std::vector<SimulationObject*>& objects,
             const unsigned int num_partitions) const;

private:
    mutable std::vector<float> part_weights_;

};

} // namespace warped

#endif
