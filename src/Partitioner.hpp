#ifndef PARTITIONER_H
#define PARTITIONER_H

#include <vector>

namespace warped {

class LogicalProcess;

class Partitioner {
public:
    virtual std::vector<std::vector<LogicalProcess*>> interNodePartition(
                                const std::vector<LogicalProcess*>& lps,
                                const unsigned int num_nodes    ) const = 0;

    virtual std::vector<std::vector<LogicalProcess*>> intraNodePartition(
                                const std::vector<LogicalProcess*>& lps ) const = 0;
};

} // namespace warped

#endif
