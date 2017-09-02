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
#include "LogicalProcess.hpp"
#include "utility/warnings.hpp"

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

    // Set PYTHONPATH to working directory
    //setenv("PYTHONPATH",".",1);

    // Convert filename to a Python string
    PyObject *p_name = PyString_FromString("LouvainPartitioner");

    // Import the file as Python module
    PyObject *p_module = PyImport_Import(p_name);

    // Create a dictionary for contents of the module
    PyObject *p_dict = PyModule_GetDict(p_module);

    // Get the add method from dictionary
    PyObject *p_func = PyDict_GetItemString(p_dict, "partition");
    assert(PyCallable_Check(p_func));

    // Create the Python object to hold arguments to the method
    PyObject *p_args = Py_BuildValue("(sI)", stats_file_.c_str(), num_nodes);

    // Call the Python function with the arguments
    PyObject *p_result = PyObject_CallObject(p_func, p_args);
    assert(p_result);
    printf("Result is %s\n", PyString_AsString(p_result));

    // Clear the Python object references
    Py_CLEAR(p_result);
    Py_CLEAR(p_args);
    Py_CLEAR(p_dict);
    Py_CLEAR(p_module);
    Py_CLEAR(p_name);

    // Destroy the Python interpreter
    Py_Finalize();

    // Store the partition data by parsing the Louvain output

    // If there is only 1 node, there is no inter-node partition
    if (num_nodes == 1) {
        return {lps};
    }

    // Split the LPs between nodes
    std::vector<std::vector<LogicalProcess*>> lps_by_node;
    return lps_by_node;
}

std::vector<std::vector<LogicalProcess*>> ProfileGuidedPartitioner::intraNodePartition(
                                    const std::vector<LogicalProcess*>& lps ) const {

    return {lps};
}

} // namespace warped
