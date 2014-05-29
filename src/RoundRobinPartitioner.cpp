#include "RoundRobinPartitioner.hpp"

#include <vector>

#include "SimulationObject.hpp"

namespace warped {

std::vector<std::vector<SimulationObject*>> RoundRobinPartitioner::partition(
                                             const std::vector<SimulationObject*>& objects,
                                             const unsigned int num_partitions) const {
    
    std::vector<std::vector<SimulationObject*>> partitions(num_partitions);

    for (unsigned int i = 0; i < objects.size(); ++i) {
        partitions[i % num_partitions].push_back(objects[i]);
    }

    return partitions;
}

} // namespace warped