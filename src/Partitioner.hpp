#ifndef PARTITIONER_H
#define PARTITIONER_H

#include <vector>

namespace warped {

class LogicalProcess;

class Partitioner {
public:
    virtual ~Partitioner() = default;
    virtual std::vector<std::vector<LogicalProcess*>> partition(
             const std::vector<LogicalProcess*>& lps,
             const unsigned int num_partitions) const = 0;
};

} // namespace warped

#endif
