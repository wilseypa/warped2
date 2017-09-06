#ifndef PROFILE_GUIDED_PARTITIONER_HPP
#define PROFILE_GUIDED_PARTITIONER_HPP

#include <iosfwd>
#include <vector>
#include <string>

#include "LogicalProcess.hpp"
#include "Partitioner.hpp"

namespace warped {

class LogicalProcess;

class ProfileGuidedPartitioner : public Partitioner {
public:
    ProfileGuidedPartitioner(std::string stats_file)
            : stats_file_(stats_file) {
        lps_ = nullptr;
    }

    std::vector<std::vector<LogicalProcess*>> interNodePartition(
                                const std::vector<LogicalProcess*>& lps,
                                const unsigned int num_nodes    ) const;

    std::vector<std::vector<LogicalProcess*>> intraNodePartition(
                                const std::vector<LogicalProcess*>& lps ) const;

private:
    std::string stats_file_;
    mutable std::vector<std::pair<unsigned int, LogicalProcess*> > *lps_;
};

} // namespace warped


#endif
