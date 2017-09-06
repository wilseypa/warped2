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

#include <Python.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "Partitioner.hpp"
#include "utility/warnings.hpp"
#include "utility/memory.hpp"

namespace warped {

std::vector<std::vector<LogicalProcess*>> ProfileGuidedPartitioner::interNodePartition(
        const std::vector<LogicalProcess*>& lps, const unsigned int num_nodes ) const {

    assert (num_nodes && num_nodes <= lps.size());

    // Create a map for Louvain vertex id to LP pointer (0 to LP_count-1)
    // Louvain vertices numbered in increasing order of LP names
    std::map<std::string, LogicalProcess*> lps_by_name;
    for (auto lp: lps) {
        lps_by_name[lp->name_] = lp;
    }
    std::vector<LogicalProcess*> lps_by_louvain;
    lps_by_louvain.reserve(lps.size());
    for (auto entry : lps_by_name) {
        lps_by_louvain.push_back(entry.second);
    }

    /* Partition using Louvain */

    // Initialize the Python interpreter
    Py_Initialize();

    // Convert filename to a Python string
    PyObject *p_name = PyString_FromString("LouvainPartitioner");
    assert(p_name);

    // Import the file as Python module
    PyObject *p_module = PyImport_Import(p_name);
    assert(p_module);

    // Create a dictionary for contents of the module
    PyObject *p_dict = PyModule_GetDict(p_module);
    assert(p_dict);

    // Get the add method from dictionary
    PyObject *p_func = PyDict_GetItemString(p_dict, "partition");
    assert(PyCallable_Check(p_func));

    // Create the Python object to hold arguments to the method
    PyObject *p_args = Py_BuildValue("(sI)", stats_file_.c_str(), num_nodes);
    assert(p_args);

    // Call the Python function with the arguments
    PyObject *p_result = PyObject_CallObject(p_func, p_args);
    assert(p_result);
    std::string lps_stream( PyString_AsString(p_result) );

    // Clear the Python object references
    Py_CLEAR(p_result);
    Py_CLEAR(p_args);
    Py_CLEAR(p_dict);
    Py_CLEAR(p_module);
    Py_CLEAR(p_name);

    // Destroy the Python interpreter
    Py_Finalize();

    /* Store the partition data by parsing the Louvain output */
    // Parse the inter-node partitions
    std::string inter_node_delimiter = "-";
    std::vector<std::string> inter_node_partitions;
    size_t pos = 0;
    while ((pos = lps_stream.find(inter_node_delimiter)) != std::string::npos) {
        inter_node_partitions.push_back( lps_stream.substr(0, pos) );
        lps_stream.erase(0, pos + inter_node_delimiter.length());
    }
    inter_node_partitions.push_back(lps_stream);

    // Parse the intra-node partitions
    std::string intra_node_delimiter = ";";
    std::string lp_delimiter = ",";
    lps_ = new std::vector<std::pair<unsigned int, LogicalProcess*>>[inter_node_partitions.size()];
    std::vector<std::vector<LogicalProcess*>> lps_by_node(inter_node_partitions.size());

    for (unsigned int node_index = 0; node_index < inter_node_partitions.size(); node_index++) {
        std::string node_str = inter_node_partitions[node_index];
        std::vector<std::string> intra_node_partitions;
        while ((pos = node_str.find(intra_node_delimiter)) != std::string::npos) {
            intra_node_partitions.push_back( node_str.substr(0, pos) );
            node_str.erase(0, pos + intra_node_delimiter.length());
        }
        intra_node_partitions.push_back(node_str);

        // Parse the LPs
        std::vector<std::pair<unsigned int, LogicalProcess*> > partition;
        LogicalProcess *lp = nullptr;

        for (unsigned int part_index = 0; part_index < intra_node_partitions.size(); part_index++) {
            std::string part_str = intra_node_partitions[part_index];
            while ((pos = part_str.find(lp_delimiter)) != std::string::npos) {
                lp = lps_by_louvain[std::stoul(part_str.substr(0, pos))];
                partition.push_back(std::make_pair(part_index, lp));
                lps_by_node[node_index].push_back(lp);
                part_str.erase(0, pos + lp_delimiter.length());
            }
            lp = lps_by_louvain[std::stoul(part_str.substr(0, pos))];
            partition.push_back(std::make_pair(part_index, lp));
            lps_by_node[node_index].push_back(lp);
        }
        lps_[node_index] = partition;
    }

    return lps_by_node;
}

std::vector<std::vector<LogicalProcess*>> ProfileGuidedPartitioner::intraNodePartition(
                                    const std::vector<LogicalProcess*>& lps ) const {

    /* Note : LPs inside 'lps' should have the same order as LPs stored for a node inside 'lps_'.
       This is because the order is derived from interNodePartition() for both structures. This
       makes the search for node index easy.
     */
    unsigned int node_index = 0;
    while (node_index < lps_->size()) {
        if (lps_[node_index].begin()->second == lps[0]) break;
        node_index++;
    }
    assert(node_index < lps_->size());

    /* Since I don't know how many partitions are there at the time of declaration,
       I elected to each partititon has one LP each. This estimate will be downgraded
       later based on vector traversal.
     */
    std::vector<std::vector<LogicalProcess*>> lps_by_partition(lps.size());
    unsigned int num_partitions = 0;
    for (unsigned int i = 0; i < lps_[node_index].size(); i++) {
        assert(lps_[node_index][i].second == lps[i]);
        unsigned int partition_index = lps_[node_index][i].first;
        num_partitions = std::max(num_partitions, partition_index+1);
        lps_by_partition[partition_index].push_back(lps[i]);
    }
    lps_by_partition.resize(num_partitions);
    lps_by_partition.shrink_to_fit();

    return lps_by_partition;
}

} // namespace warped
