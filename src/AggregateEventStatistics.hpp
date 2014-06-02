#ifndef AGGREGATE_EVENT_STATISTICS_HPP
#define AGGREGATE_EVENT_STATISTICS_HPP

#include <iosfwd>
#include <map>
#include <string>
#include <utility>

#include "EventStatistics.hpp"

namespace warped {

class Event;

// Collects a count of events sent between objects. The objects and events are
// represented as an undirected graph with weighted edges such that the weight
// of an edge is the count of events sent between objects incident that edge.
class AggregateEventStatistics : public EventStatistics {
public:
    enum class OutputType {Graphviz, Metis};
    typedef std::pair<std::string, std::string> Edge;
    typedef unsigned int stat_type;

    AggregateEventStatistics(std::string filename, OutputType output_type);

    void record(const std::string& source, unsigned int send_time, Event* event);

    // Make an undirectional edge connecting two vertices.
    Edge makeEdge(const std::string& a, const std::string& b) const;

    // Return the count of events along the edge e.
    unsigned int getStat(const Edge& e) const;

private:
    OutputType output_type_;
    std::map<Edge, stat_type> edge_stats_;

    // Print stats by delegating to the different output formats
    std::ostream& printStats(std::ostream& stream) const;
    std::ostream& printGraphviz(std::ostream& stream) const;
    std::ostream& printMetis(std::ostream& stream) const;
};

} // namespace warped

#endif
