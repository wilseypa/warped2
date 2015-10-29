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
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "Partitioner.hpp"
#include "LogicalProcess.hpp"

namespace warped {

ProfileGuidedPartitioner::ProfileGuidedPartitioner(std::string stats_file, std::string output_prefix) :
    stats_file_(stats_file), output_prefix_(output_prefix) {}

std::vector<std::vector<LogicalProcess*>> ProfileGuidedPartitioner::partition(
const std::vector<LogicalProcess*>& lps, const unsigned int num_partitions) const {

    if (num_partitions == 1) {
        return {lps};
    }

    std::ifstream input(stats_file_);
    if (!input.is_open()) {
        throw std::runtime_error(std::string("Could not open statistics file ") + stats_file_);
    }

    std::vector<std::vector<LogicalProcess*>>   lps_by_partition(num_partitions);
    std::vector<std::vector<idx_t>>             xadj_by_partition(num_partitions);
    std::vector<std::vector<idx_t>>             adjncy_by_partition(num_partitions);
    std::vector<std::vector<idx_t>>             adjwgt_by_partition(num_partitions);
    std::vector<std::vector<unsigned int>>      numbering_by_partition(num_partitions);

    // A map of METIS node number -> LogicalProcess name that is used to
    // translate the metis partition info to warped partitions.
    std::vector<std::string> lp_names(lps.size());

    // METIS parameters
    // idx_t is a METIS typedef
    idx_t nvtxs = 0; // number of verticies
    idx_t ncon = 1; // number of constraints
    idx_t nparts = num_partitions; // number of partitions
    std::vector<idx_t> xadj; // part of the edge list
    std::vector<idx_t> adjncy; // part of the edge list
    std::vector<idx_t> adjwgt; // edge weights
    idx_t edgecut = 0; // output var for the final communication volume
    std::vector<idx_t> part(lps.size()); // output var for partition list

    xadj.push_back(0);
    int i = 0;
    for (std::string line; std::getline(input, line);) {
        if (line[0] == '%') {
            // Skip comment lines unless they are a map comment
            if (line[1] != ':') { continue; }
            // The first line is the file format, so store the name at i-1
            lp_names[i - 1] = line.substr(3);
            continue;
        }

        std::istringstream iss(line);
        if (i == 0) {
            int nedges, format;
            iss >> nvtxs >> nedges >> format;

            if (format != 1) {
                throw std::runtime_error("Graph format must be 001.");
            }
            if (static_cast<std::size_t>(nvtxs) > lps.size()) {
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
                        NULL,           // tpwgts
                        NULL,           // ubvec
                        NULL,           // options
                        &edgecut,       // edgecut
                        &part[0]        // part
                       );

    // Create a map of all LogicalProcess names -> pointers
    std::unordered_map<std::string, LogicalProcess*> lps_by_name;
    for (auto lp: lps) {
        lps_by_name[lp->name_] = lp;
    }

    // Add the metis output to partitons, removing partitioned names from the
    // lp map. Any left over lps don't have profiling data associated,
    // and will be partitioned uniformly.

    for (unsigned int i = 0; i < num_partitions; i++) {
        xadj_by_partition[i].push_back(0);
    }

    for (int i = 0; i < nvtxs; i++) {
        auto name = lp_names[i];
        lps_by_partition[part[i]].push_back(lps_by_name.at(name));

        for (int j = xadj[i]; j < xadj[i+1]; j++) {
            if (part[i] == part[adjncy[j]]) {
                adjncy_by_partition[part[i]].push_back(adjncy[j]);
                adjwgt_by_partition[part[i]].push_back(adjwgt[j]);
            }
        }
        xadj_by_partition[part[i]].push_back(adjncy_by_partition[part[i]].size());
        numbering_by_partition[part[i]].push_back((unsigned int)i);

        lps_by_name.erase(name);
    }

    for (unsigned int i = 0; i < num_partitions; i++) {
        savePartition(i, lps_by_partition[i], xadj_by_partition[i], adjncy_by_partition[i],
                      adjwgt_by_partition[i], numbering_by_partition[i]);
    }

    // Add any lps that metis didn't partition
    unsigned int j = 0;
    for (auto& it : lps_by_name) {
        lps_by_partition[j % num_partitions].push_back(it.second);
        j++;
    }

    return lps_by_partition;
}

void ProfileGuidedPartitioner::savePartition(unsigned int part_id, const std::vector<LogicalProcess*>& lps,
const std::vector<idx_t>& xadj, const std::vector<idx_t>& adjncy, const std::vector<idx_t>& adjwgt,
const std::vector<unsigned int>& numbering) const {

    auto num_vertices = 0;
    for (unsigned int k = 0; k < lps.size(); k++) {
        if ((xadj[k+1] - xadj[k]) > 0) num_vertices++;
    }

    auto num_edges = adjncy.size() / 2;

    unsigned int i = 1;
    std::map<unsigned int, unsigned int>    new_number;
    for (auto& n : numbering) {
        new_number[n] = i++;
    }

    struct stat st;
    if (stat("partitions", &st) == -1) {
        mkdir("partitions", 0755);
    }

    std::ofstream ofs("partitions/"+output_prefix_+std::to_string(part_id)+".out",
        std::ios::trunc | std::ios::out);

    ofs << "%% The first line contains the following information:\n"
        << "%% <# of vertices> <# of edges> <file format>\n"
        <<  num_vertices << ' ' << num_edges << ' ' << "001 " << '\n'
        << "%% Lines that start with %: are comments that are ignored by Metis,\n"
        << "%% but list the WARPED lp name for the Metis node described by \n"
        << "%% the following line.\n"
        << "%% The remaining lines each describe a vertex and have the following format:\n"
        << "%% <<neighbor> <event count>> ...";

    i = 0;
    for (auto& lp : lps) {

        if ((xadj[i+1] - xadj[i]) == 0) return;

        ofs << "\n%: " << lp->name_ << '\n';
        for (idx_t j = xadj[i]; j < xadj[i+1]; j++) {
            ofs << new_number[adjncy[j]] << ' ' << adjwgt[j] << ' ';
        }

        ++i;
    }
    ofs << '\n';

    ofs.close();
}

} // namespace warped
