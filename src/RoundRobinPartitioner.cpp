#include "RoundRobinPartitioner.hpp"

#include <vector>
#include <cassert>
#include <iostream>

#include "LogicalProcess.hpp"

namespace warped {

std::vector<std::vector<LogicalProcess*>> RoundRobinPartitioner::interNodePartition(
                                             const std::vector<LogicalProcess*>& lps,
                                             const unsigned int num_nodes) const {

    std::vector<std::vector<LogicalProcess*>> partitions(num_nodes);
    assert( num_nodes && (num_nodes <= lps.size()) );
    if (num_nodes == 1) {
        partitions[0] = lps;

    } else {
        for (unsigned int i = 0; i < lps.size(); i++) {
            partitions[i % num_nodes].push_back( lps[i] );
        }
    }
    return partitions;
}

std::vector<std::vector<LogicalProcess*>> RoundRobinPartitioner::intraNodePartition(
                                            const std::vector<LogicalProcess*>& lps ) const {

    std::vector<std::vector<LogicalProcess*>> partitions;
    if (!block_size_ || block_size_ >= lps.size()) {
        partitions.push_back(lps);

    } else {
        for (unsigned int i = 0; i < lps.size(); i++) {
            partitions[i % block_size_].push_back( lps[i] );
        }
    }
    return partitions;
}

} // namespace warped
