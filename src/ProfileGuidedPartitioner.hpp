#ifndef PROFILE_GUIDED_PARTITIONER_HPP
#define PROFILE_GUIDED_PARTITIONER_HPP

#include <iosfwd>
#include <vector>
#include <string>

#include "metis/include/metis.h"

#include "Partitioner.hpp"

namespace warped {

class LogicalProcess;

class ProfileGuidedPartitioner : public Partitioner {
public:
    ProfileGuidedPartitioner(std::string stats_file, std::string output_prefix);

    std::vector<std::vector<LogicalProcess*>> partition(
             const std::vector<LogicalProcess*>& lps,
             const unsigned int num_partitions) const;

protected:
    void savePartition(unsigned int part_id,
                       const std::vector<LogicalProcess*>& lps,
                       const std::vector<idx_t>& xadj,
                       const std::vector<idx_t>& adjncy,
                       const std::vector<idx_t>& adjwgt,
                       const std::vector<unsigned int>& numbering = {}) const;

private:
    std::string stats_file_;

    std::string output_prefix_;

};

} // namespace warped


#endif
