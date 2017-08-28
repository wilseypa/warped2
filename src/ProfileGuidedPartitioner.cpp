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
    if (num_nodes == 1) {
        return {lps};
    }

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
    Py_Initialize();

    PyRun_SimpleString("import sys");
    PyRun_SimpleString("sys.path.append(\".\")");
    PyObject *p_name   = PyString_FromString((char*)"LouvainPartitioner");
    PyObject *p_module = PyImport_Import(p_name);
    PyObject *p_dict   = PyModule_GetDict(p_module);
    PyObject *p_func   = PyDict_GetItemString(p_dict, (char*)"create_comm_graph");

    PyObject *p_value = nullptr, *p_result = nullptr;
    if (PyCallable_Check(p_func)) {
        p_value  = Py_BuildValue("(z)", stats_file_.c_str());
        PyErr_Print();
        p_result = PyObject_CallObject(p_func, p_value);
        PyErr_Print();

    } else {
        PyErr_Print();
    }

    printf("Result is %d\n",PyInt_AsLong(p_result));

    Py_DECREF(p_value);
    Py_DECREF(p_result);
    Py_DECREF(p_module);
    Py_DECREF(p_name);

    Py_Finalize();

    // Build the partitions by parsing the Louvain output
    std::vector<std::vector<LogicalProcess*>> lps_by_partition;

    // Add the vertices missing from Louvain output


    return lps_by_partition;
}

std::vector<std::vector<LogicalProcess*>> ProfileGuidedPartitioner::intraNodePartition(
                                    const std::vector<LogicalProcess*>& lps ) const {

    unused(lps);
    std::vector<std::vector<LogicalProcess*>> lps_by_partition;
    return lps_by_partition;
}

} // namespace warped
