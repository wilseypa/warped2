#ifndef ROUND_ROBIN_PARTITIONER_HPP
#define ROUND_ROBIN_PARTITIONER_HPP

#include <vector>

#include "Partitioner.hpp"

namespace warped {

class LogicalProcess;

class RoundRobinPartitioner : public Partitioner {
public:
    RoundRobinPartitioner(std::vector<float> part_weights = {}) : part_weights_(part_weights) {}

    std::vector<std::vector<LogicalProcess*>> partition(
             const std::vector<LogicalProcess*>& lps,
             const unsigned int num_partitions) const;

private:
    mutable std::vector<float> part_weights_;

};

} // namespace warped

#endif
