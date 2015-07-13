#include "RoundRobinPartitioner.hpp"

#include <vector>
#include <cmath>

#include "SimulationObject.hpp"

namespace warped {

std::vector<std::vector<SimulationObject*>> RoundRobinPartitioner::partition(
                                             const std::vector<SimulationObject*>& objects,
                                             const unsigned int num_partitions) const {

    if (part_weights_.empty()) {
        part_weights_.assign(num_partitions, 1.0/num_partitions);
    } else {
        if (part_weights_.size() != num_partitions) {
            throw std::runtime_error("The number of weights must equal the number of partitions!"); 
        }

        float weight_sum = 0.0;
        for (auto& w : part_weights_) {
            weight_sum += w;
        }
        if (weight_sum != 1.0) {
            throw std::runtime_error("The sum of partition weights must equal 1.0!");
        }
    }

    std::vector<unsigned int> num_objects_by_partition;
    for (auto& w : part_weights_) {
        num_objects_by_partition.push_back(std::ceil(w*objects.size()));
    }

    std::vector<std::vector<SimulationObject*>> partitions(num_partitions);

    for (unsigned int i = 0, j = 0; j < objects.size(); ++i) {
        if (partitions[i % num_partitions].size() < num_objects_by_partition[i % num_partitions]) {
            partitions[i % num_partitions].push_back(objects[j]);
            j++;
        }
    }

    return partitions;
}

} // namespace warped
