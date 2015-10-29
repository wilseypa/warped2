#include "RoundRobinPartitioner.hpp"

#include <vector>
#include <iostream>

#include "LogicalProcess.hpp"

namespace warped {

std::vector<std::vector<LogicalProcess*>> RoundRobinPartitioner::partition(
                                             const std::vector<LogicalProcess*>& lps,
                                             const unsigned int num_partitions) const {

    std::vector<std::vector<LogicalProcess*>> partitions(num_partitions);

    if (block_size_ == 0) {
        block_size_ = lps.size()/num_partitions;
    }

    unsigned int num_blocks = lps.size()/block_size_;

    unsigned int i = 0, j = 0;
    for (i = 0; i < num_blocks; i++) {
        for (j = 0; j < block_size_; j++) {
            partitions[i % num_partitions].push_back(lps[block_size_*i+j]);
        }
    }

    i--;
    while ((block_size_*i+j) < lps.size()) {
        partitions[j % num_partitions].push_back(lps[block_size_*i+j]);
        j++;
    }

    return partitions;
}

} // namespace warped
