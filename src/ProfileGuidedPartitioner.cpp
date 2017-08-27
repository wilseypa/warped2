#include "ProfileGuidedPartitioner.hpp"

#include <cstddef>
#include <iostream>
#include <sstream>
#include <fstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>
#include <cassert>

#include <Python.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "Partitioner.hpp"
#include "LogicalProcess.hpp"

namespace warped {

ProfileGuidedPartitioner::ProfileGuidedPartitioner(
            std::string stats_file,
            std::string output_prefix   ) :
    stats_file_(stats_file),
    output_prefix_(output_prefix) {}

std::vector<std::vector<LogicalProcess*>> ProfileGuidedPartitioner::partition(
                                            const std::vector<LogicalProcess*>& lps,
                                            const unsigned int num_partitions   ) const {

    assert (num_partitions && num_partitions <= lps.size());
    if (num_partitions == 1) {
        return {lps};
    }

    // Create a map for Louvain vertex id to LP pointer (0 to LP_count-1)
    // Louvain vertices numbered in increasing order of LP names
    std::map<std::string, LogicalProcess*> lps_by_name;
    for (auto lp: lps) {
        lps_by_name[lp->name_] = lp;
    }
    std::vector<LogicalProcess*> lps_by_louvain;
    lps_by_louvain.reserve(lps.size());
    for (auto entry : lps_by_name) {
        lps_by_louvain.push_back(entry.second);
    }

    // Partition using Louvain

    // Build the partitions by parsing the Louvain output
    std::vector<std::vector<LogicalProcess*>> lps_by_partition;

    // Add the vertices missing from Louvain output


    return lps_by_partition;
}

} // namespace warped
