#include "ProfileGuidedPartitioner.hpp"

#include <cstddef>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#include "metis/include/metis.h"

#include "Partitioner.hpp"
#include "SimulationObject.hpp"

namespace warped {

ProfileGuidedPartitioner::ProfileGuidedPartitioner(std::istream& input)
    : input(input) {}

std::vector<std::vector<SimulationObject*>> ProfileGuidedPartitioner::partition(
const std::vector<SimulationObject*>& objects, const unsigned int num_partitions) const {
    if (num_partitions == 1) {
        return {objects};
    }

    if (!input) {
        throw std::runtime_error("Could not read file.");
    }

    std::vector<std::vector<SimulationObject*>> partitions(num_partitions);

    // A map of METIS node number -> SimulationObject name that is used to
    // translate the metis partition info to warped partitions.
    std::vector<std::string> object_names(objects.size());

    // METIS parameters
    // idx_t is a METIS typedef
    idx_t nvtxs; // number of verticies
    idx_t ncon = 1; // number of constraints
    idx_t nparts = num_partitions; // number of partitions
    std::vector<idx_t> xadj; // part of the edge list
    std::vector<idx_t> adjncy; // part of the edge list
    std::vector<idx_t> adjwgt; // edge weights
    idx_t edgecut; // output var for the final communication volume
    std::vector<idx_t> part(objects.size()); // output var for partition list

    xadj.push_back(0);
    int i = 0;
    for (std::string line; std::getline(input, line);) {
        if (line[0] == '%') {
            // Skip comment lines unless they are a map comment
            if (line[1] != ':') { continue; }
            // The first line is the file format, so store the name at i-1
            object_names[i - 1] = line.substr(3);
            continue;
        }

        std::istringstream iss(line);
        if (i == 0) {
            unsigned int nedges, format;
            iss >> nvtxs >> nedges >> format;
            if (format != 1) {
                throw std::runtime_error("Graph format must be 001.");
            }
            if (static_cast<std::size_t>(nvtxs) > objects.size()) {
                throw std::runtime_error("Invalid statistics file for this simulation.");
            }
        } else {
            unsigned int vertex, weight;
            while (iss >> vertex >> weight) {
                // The metis file format counts from 1, but the API counts from 0. Cool.
                adjncy.push_back(vertex - 1);
                adjwgt.push_back(weight);
            }
            xadj.push_back(adjncy.size());
        }
        i++;
    }

    METIS_PartGraphKway(&nvtxs,     // nvtxs
                        &ncon,      // ncon
                        &xadj[0],   // xadj
                        &adjncy[0], // adjncy
                        NULL,       // vwgt
                        NULL,       // vsize
                        &adjwgt[0], // adjwgt
                        &nparts,    // nparts
                        NULL,       // tpwgts
                        NULL,       // ubvec
                        NULL,        // options
                        &edgecut,   // edgecut
                        &part[0]    // part
                       );

    // Create a map of all SimulationObject names -> pointers
    std::unordered_map<std::string, SimulationObject*> objects_by_name;
    for (auto ob: objects) {
        objects_by_name[ob->name_] = ob;
    }

    // Add the metis output to partitons, removing partitioned names from the
    // object map. Any left over objects don't have profiling data associated,
    // and will be partitioned uniformly.
    for (int i = 0; i < nvtxs; i++) {
        auto name = object_names[i];
        partitions[part[i]].push_back(objects_by_name.at(name));
        objects_by_name.erase(name);
    }

    // Add any objects that metis didn't partition
    unsigned int j = 0;
    for (auto& it : objects_by_name) {
        partitions[j % num_partitions].push_back(it.second);
        j++;
    }

    return partitions;
}

} // namespace warped
