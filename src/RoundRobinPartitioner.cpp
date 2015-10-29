#include "RoundRobinPartitioner.hpp"

#include <vector>
#include <cmath>
#include <stdexcept>

#include "LogicalProcess.hpp"

namespace warped {

std::vector<std::vector<LogicalProcess*>> RoundRobinPartitioner::partition(
                                             const std::vector<LogicalProcess*>& lps,
                                             const unsigned int num_partitions) const {

    std::vector<std::vector<LogicalProcess*>> partitions(num_partitions);

    for (unsigned int i = 0; i < lps.size(); ++i) {
        partitions[i % num_partitions].push_back(lps[i]);
    }

    return partitions;
}

} // namespace warped
