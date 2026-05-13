#pragma once

#include "petri/common/result.hpp"
#include "petri/core_pn/model.hpp"
#include "petri/core_pn/petri_net.hpp"
#include "petri/graph_models/directed_graph.hpp"

#include <map>
#include <string>

namespace petri {

struct ReachabilityOptions {
    std::size_t max_states = 1000;
    std::size_t max_depth = 20;
};

struct ReachabilityGraph {
    DirectedGraph graph;
    std::string initial_vertex_id;
    std::map<std::string, Marking> markings;
    std::map<std::string, std::size_t> depths;
};

Result<ReachabilityGraph> build_reachability_graph(const PetriNet& net, const ReachabilityOptions& options);
Result<std::string> find_marking_vertex(const PetriNet& net, const ReachabilityGraph& reachability,
                                        const nlohmann::json& partial_marking);
nlohmann::json reachability_graph_to_json(const PetriNet& net, const ReachabilityGraph& reachability);

} // namespace petri
