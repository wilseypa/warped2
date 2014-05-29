#ifndef PARTITIONER_H
#define PARTITIONER_H

#include <vector>

namespace warped {

class SimulationObject;

class Partitioner {
public:
    virtual std::vector<std::vector<SimulationObject*>> partition(
             const std::vector<SimulationObject*>& objects,
             const unsigned int num_partitions) const = 0;
};

} // namespace warped

#endif