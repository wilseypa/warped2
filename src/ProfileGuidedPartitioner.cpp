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

#include "metis/include/metis.h"

#include "Partitioner.hpp"
#include "SimulationObject.hpp"

namespace warped {

ProfileGuidedPartitioner::ProfileGuidedPartitioner(std::string stats_file,
    std::vector<float> part_weights) : stats_file_(stats_file), part_weights_(part_weights) {}

std::vector<std::vector<SimulationObject*>> ProfileGuidedPartitioner::partition(
const std::vector<SimulationObject*>& objects, const unsigned int num_partitions) const {

    if (num_partitions == 1) {
        return {objects};
    }

    if (part_weights_.empty()) {
        part_weights_.assign(num_partitions, (float)1.0/num_partitions);
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

    std::ifstream input(stats_file_);
    if (!input.is_open()) {
        throw std::runtime_error(std::string("Could not open statistics file ") + stats_file_);
    }

    std::vector<std::vector<SimulationObject*>> objects_by_partition(num_partitions);
    std::vector<std::vector<idx_t>>             xadj_by_partition(num_partitions);
    std::vector<std::vector<idx_t>>             adjncy_by_partition(num_partitions);
    std::vector<std::vector<idx_t>>             adjwgt_by_partition(num_partitions);
    std::vector<std::vector<unsigned int>>      numbering_by_partition(num_partitions);

    // A map of METIS node number -> SimulationObject name that is used to
    // translate the metis partition info to warped partitions.
    std::vector<std::string> object_names(objects.size());

    // METIS parameters
    // idx_t is a METIS typedef
    idx_t nvtxs = 0; // number of verticies
    idx_t ncon = 1; // number of constraints
    idx_t nparts = num_partitions; // number of partitions
    std::vector<idx_t> xadj; // part of the edge list
    std::vector<idx_t> adjncy; // part of the edge list
    std::vector<idx_t> adjwgt; // edge weights
    idx_t edgecut = 0; // output var for the final communication volume
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
            int nedges, format;
            iss >> nvtxs >> nedges >> format;

            if (format != 1) {
                throw std::runtime_error("Graph format must be 001.");
            }
            if (static_cast<std::size_t>(nvtxs) > objects.size()) {
                throw std::runtime_error("Invalid statistics file for this simulation.");
            }
        } else {
            int vertex, weight;
            while (iss >> vertex >> weight) {
                // The metis file format counts from 1, but the API counts from 0. Cool.
                adjncy.push_back(vertex - 1);
                adjwgt.push_back(weight);
            }
            xadj.push_back(adjncy.size());
        }
        i++;
    }
    input.close();

    METIS_PartGraphKway(&nvtxs,         // nvtxs
                        &ncon,          // ncon
                        &xadj[0],       // xadj
                        &adjncy[0],     // adjncy
                        NULL,           // vwgt
                        NULL,           // vsize
                        &adjwgt[0],     // adjwgt
                        &nparts,        // nparts
                        &part_weights_[0],// tpwgts
                        NULL,           // ubvec
                        NULL,           // options
                        &edgecut,       // edgecut
                        &part[0]        // part
                       );

    // Create a map of all SimulationObject names -> pointers
    std::unordered_map<std::string, SimulationObject*> objects_by_name;
    for (auto ob: objects) {
        objects_by_name[ob->name_] = ob;
    }

    // Add the metis output to partitons, removing partitioned names from the
    // object map. Any left over objects don't have profiling data associated,
    // and will be partitioned uniformly.

    for (unsigned int i = 0; i < num_partitions; i++) {
        xadj_by_partition[i].push_back(0);
    }

    for (int i = 0; i < nvtxs; i++) {
        auto name = object_names[i];
        objects_by_partition[part[i]].push_back(objects_by_name.at(name));

        for (int j = xadj[i]; j < xadj[i+1]; j++) {
            if (part[i] == part[adjncy[j]]) {
                adjncy_by_partition[part[i]].push_back(adjncy[j]);
                adjwgt_by_partition[part[i]].push_back(adjwgt[j]);
            }
        }
        xadj_by_partition[part[i]].push_back(adjncy_by_partition[part[i]].size());
        numbering_by_partition[part[i]].push_back((unsigned int)i);

        objects_by_name.erase(name);
    }

    // Add any objects that metis didn't partition
    unsigned int j = 0;
    for (auto& it : objects_by_name) {
        objects_by_partition[j % num_partitions].push_back(it.second);
        j++;
    }

    for (unsigned int i = 0; i < num_partitions; i++) {
        savePartition(i, objects_by_partition[i], xadj_by_partition[i], adjncy_by_partition[i],
                      adjwgt_by_partition[i], numbering_by_partition[i]);
    }

    return objects_by_partition;
}

void ProfileGuidedPartitioner::savePartition(unsigned int part_id, const std::vector<SimulationObject*>& objects,
const std::vector<idx_t>& xadj, const std::vector<idx_t>& adjncy, const std::vector<idx_t>& adjwgt,
const std::vector<unsigned int>& numbering) const {

    auto num_vertices = 0;
    for (unsigned int k = 0; k < objects.size(); k++) {
        if ((xadj[k+1] - xadj[k]) > 0) num_vertices++;
    }

    auto num_edges = adjncy.size() / 2;

    unsigned int i = 1;
    std::map<unsigned int, unsigned int>    new_number;
    for (auto& n : numbering) {
        new_number[n] = i++;
    }

    std::ofstream ofs("partition"+std::to_string(part_id)+".out", std::ios::trunc | std::ios::out);

    ofs << "%% The first line contains the following information:\n"
        << "%% <# of vertices> <# of edges> <file format>\n"
        <<  num_vertices << ' ' << num_edges << ' ' << "001 " << '\n'
        << "%% Lines that start with %: are comments that are ignored by Metis,\n"
        << "%% but list the WARPED object name for the Metis node described by \n"
        << "%% the following line.\n"
        << "%% The remaining lines each describe a vertex and have the following format:\n"
        << "%% <<neighbor> <event count>> ...";

    i = 0;
    for (auto& ob : objects) {

        if ((xadj[i+1] - xadj[i]) == 0) return;

        ofs << "\n%: " << ob->name_ << '\n';
        for (idx_t j = xadj[i]; j < xadj[i+1]; j++) {
            ofs << new_number[adjncy[j]] << ' ' << adjwgt[j] << ' ';
        }

        ++i;
    }
    ofs << '\n';

    ofs.close();
}

} // namespace warped
