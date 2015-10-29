#ifndef ROUND_ROBIN_PARTITIONER_HPP
#define ROUND_ROBIN_PARTITIONER_HPP

#include <vector>

#include "Partitioner.hpp"

namespace warped {

class LogicalProcess;

class RoundRobinPartitioner : public Partitioner {
public:
    RoundRobinPartitioner() = default;

    std::vector<std::vector<LogicalProcess*>> partition(
             const std::vector<LogicalProcess*>& lps,
             const unsigned int num_partitions) const;

};

} // namespace warped

#endif
