#include <algorithm>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <utility>

#include "AggregateEventStatistics.hpp"
#include "Event.hpp"
#include "EventStatistics.hpp"
#include "utility/warnings.hpp"

namespace warped {

AggregateEventStatistics::Edge AggregateEventStatistics::makeEdge(const std::string& a,
                                                                  const std::string& b) const {
    return (a <= b) ? std::make_pair(a, b) : std::make_pair(b, a);
}

AggregateEventStatistics::AggregateEventStatistics(std::string filename, OutputType output_type)
    : EventStatistics(std::move(filename)), output_type_(output_type) {}


void AggregateEventStatistics::record(const std::string& source, unsigned int send_time,
                                      Event* event) {
    unused(send_time);
    edge_stats_[makeEdge(source, event->receiverName())] += 1;
}

unsigned int AggregateEventStatistics::getStat(const Edge& e) const {
    return edge_stats_.at(e);
}

std::ostream& AggregateEventStatistics::printStats(std::ostream& stream) const {
    if (output_type_ == OutputType::Graphviz) {
        return printGraphviz(stream);
    }
    return printMetis(stream);
}

std::ostream& AggregateEventStatistics::printGraphviz(std::ostream& stream) const {
    stream << "graph {\n";
    for (const auto& stats_it : edge_stats_) {
        stream << '"' << stats_it.first.first << "\" -- \"" << stats_it.first.second
               << "\" [label=\"" << stats_it.second << "\"];\n";
    }
    stream << "}\n";
    return stream;
}

std::ostream& AggregateEventStatistics::printMetis(std::ostream& stream) const {
    // Metis files require a list of every edge for every node, so we have to
    // build a map of edges in both directions.
    std::map<Edge, stat_type> full_graph;

    // Count the vertices
    std::set<std::string> vertices;

    // Build the directed graph
    for (const auto& stats_it : edge_stats_) {
        // Metis doesn't support self-edges
        if (stats_it.first.first == stats_it.first.second) { continue; }
        vertices.insert(stats_it.first.first);
        vertices.insert(stats_it.first.second);
        // Record the edge in both directions
        full_graph[stats_it.first] = stats_it.second;
        full_graph[std::make_pair(stats_it.first.second, stats_it.first.first)] = stats_it.second;
    }

    if (vertices.size() == 0) {
        std::cerr << "Cannot create statistics file, no events were recorded" << std::endl;
        return stream;
    }

    const auto num_vertices = vertices.size();
    const auto num_edges = full_graph.size() / 2;
    stream << "%% The first line contains the following information:\n"
           << "%% <# of vertices> <# of edges> <file format>\n"
           <<  num_vertices << ' ' << num_edges << ' ' << "001 " << '\n'
           << "%% Lines that start with %: are comments that are ignored by Metis,\n"
           << "%% but list the WARPED object name for the Metis node described by \n"
           << "%% the following line.\n"
           << "%% The remaining lines each describe a vertex and have the following format:\n"
           << "%% <<neighbor> <event count>> ...";

    // Metis requires vertex numbering start at 1 and be contiguous, so we
    // have to map the object names to numbers. This also takes care of the
    // case where there is a vertex with no edges.
    std::map<std::string, unsigned int> numbering;
    unsigned int i = 1;
    for (const auto& vertex : vertices) {
        numbering[vertex] = i++;
    }

    // Keep track of the last vertex, since each vertex is on its own line
    std::string last_vertex;
    // The std::map is ordered, so iterating over the edges will return the
    // edges for each nodes sequentially
    for (const auto& edges_it : full_graph) {
        if (edges_it.first.first != last_vertex) {
            // Print the original node number as a comment so WARPED can use
            // the partition info.
            stream << "\n%: " << edges_it.first.first << '\n';
            last_vertex = edges_it.first.first;
        }

        stream << numbering[edges_it.first.second] << ' ';

        // Metis requires weights be non-zero
        auto weight = std::max(1u, edges_it.second);
        stream << weight << ' ';
    }
    return stream << '\n';
}

} // namespace warped
