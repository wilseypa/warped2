#ifndef PROFILE_GUIDED_PARTITIONER_HPP
#define PROFILE_GUIDED_PARTITIONER_HPP

#include <iosfwd>
#include <vector>
#include <string>

#include "Partitioner.hpp"

namespace warped {

class LogicalProcess;

class ProfileGuidedPartitioner : public Partitioner {
public:
    ProfileGuidedPartitioner(std::string stats_file, std::string output_prefix);

    std::vector<std::vector<LogicalProcess*>> partition(
             const std::vector<LogicalProcess*>& lps,
             const unsigned int num_partitions) const;

private:
    std::string stats_file_;

    std::string output_prefix_;

};

} // namespace warped


#endif
