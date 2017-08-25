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
    if (event) {
        edge_stats_[makeEdge(source, event->receiverName())] += 1;
    } else {
        edge_stats_[makeEdge(source, source)] += 0;
    }
}

unsigned int AggregateEventStatistics::getStat(const Edge& e) const {
    return edge_stats_.at(e);
}

std::ostream& AggregateEventStatistics::printStats(std::ostream& stream) const {
    return (output_type_ == OutputType::Graphviz) ? printGraphviz(stream) : printLouvain(stream);
}

std::ostream& AggregateEventStatistics::printGraphviz(std::ostream& stream) const {
    stream << "graph {\n";
    for (const auto& edge : edge_stats_) {
        if (!edge.second) continue;
        stream  << '"' << edge.first.first
                << "\" -- \"" << edge.first.second
                << "\" [label=\"" << edge.second
                << "\"];\n";
    }
    stream << "}\n";
    return stream;
}

std::ostream& AggregateEventStatistics::printLouvain(std::ostream& stream) const {

    std::map<Edge, stat_type> full_graph;

    // Count the vertices
    std::set<std::string> vertices;
    for (const auto& edge : edge_stats_) {
        vertices.insert(edge.first.first);
        vertices.insert(edge.first.second);
        if ( !edge.second || (edge.first.first == edge.first.second) ) continue;
        full_graph[edge.first] = edge.second;
    }

    if (vertices.size() == 0) {
        std::cerr << "Cannot create statistics file, no events were recorded" << std::endl;
        return stream;
    }

    // Vertex numbering starts at 1 and is contiguous, so we map the
    // object names to numbers. This also takes care of the case where
    // there is a vertex with no edges.
    std::map<std::string, unsigned int> numbering;
    unsigned int i = 1;
    for (const auto& vertex : vertices) {
        numbering[vertex] = i++;
    }

    // Feed data to the output stream
    stream << "LP_x,LP_y,Frequency" << std::endl;
    for (const auto& edge : edge_stats_) {
        stream << numbering[edge.first.first]  << ","
               << numbering[edge.first.second] << ","
               << edge.second                  << std::endl;
    }
    return stream;
}

} // namespace warped
