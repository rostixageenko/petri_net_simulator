#pragma once

#include "petri/common/result.hpp"
#include "petri/graph_models/directed_graph.hpp"
#include "petri/graph_models/reachability.hpp"

#include <nlohmann/json.hpp>

#include <cstddef>
#include <string>
#include <vector>

namespace petri {

struct AdjacencyListEdge {
    std::string edge_id;
    std::string target_vertex_id;
    std::size_t target_index = 0;
    std::string label;
    double weight = 1.0;
};

struct AdjacencyListRow {
    std::string vertex_id;
    std::vector<AdjacencyListEdge> edges;
};

struct AdjacencyList {
    std::vector<std::string> vertex_ids;
    std::vector<AdjacencyListRow> rows;
};

struct ForwardStarForm {
    std::vector<std::string> vertex_ids;
    std::vector<std::size_t> row_offsets;
    std::vector<std::string> edge_ids;
    std::vector<std::string> target_vertex_ids;
    std::vector<std::size_t> target_indices;
    std::vector<std::string> labels;
    std::vector<double> weights;
};

struct BackwardStarForm {
    std::vector<std::string> vertex_ids;
    std::vector<std::size_t> row_offsets;
    std::vector<std::string> edge_ids;
    std::vector<std::string> source_vertex_ids;
    std::vector<std::size_t> source_indices;
    std::vector<std::string> labels;
    std::vector<double> weights;
};

AdjacencyList adjacency_list_from_graph(const DirectedGraph& graph);
ForwardStarForm forward_star_from_graph(const DirectedGraph& graph);
BackwardStarForm backward_star_from_graph(const DirectedGraph& graph);

AdjacencyList adjacency_list_from_reachability_graph(const ReachabilityGraph& reachability);
ForwardStarForm forward_star_from_reachability_graph(const ReachabilityGraph& reachability);
BackwardStarForm backward_star_from_reachability_graph(const ReachabilityGraph& reachability);

nlohmann::json adjacency_list_to_json(const AdjacencyList& adjacency);
Result<AdjacencyList> adjacency_list_from_json(const nlohmann::json& data);

nlohmann::json forward_star_to_json(const ForwardStarForm& form);
Result<ForwardStarForm> forward_star_from_json(const nlohmann::json& data);

nlohmann::json backward_star_to_json(const BackwardStarForm& form);
Result<BackwardStarForm> backward_star_from_json(const nlohmann::json& data);

} // namespace petri
