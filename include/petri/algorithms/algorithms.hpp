#pragma once

#include "petri/common/result.hpp"
#include "petri/graph_models/directed_graph.hpp"

#include <optional>
#include <string>
#include <vector>

namespace petri {

struct AlgorithmParams {
    std::string source_vertex_id;
    std::optional<std::string> target_vertex_id;
};

struct AlgorithmMetrics {
    double elapsed_ms = 0.0;
    std::size_t visited_vertices = 0;
    std::size_t scanned_edges = 0;
    std::size_t path_length = 0;
    double path_cost = 0.0;
};

struct AlgorithmResult {
    std::string algorithm;
    std::string status;
    bool found = false;
    std::vector<std::string> path;
    AlgorithmMetrics metrics;
};

Result<AlgorithmResult> run_dfs(const DirectedGraph& graph, const AlgorithmParams& params);
Result<AlgorithmResult> run_bfs(const DirectedGraph& graph, const AlgorithmParams& params);
Result<AlgorithmResult> run_dijkstra(const DirectedGraph& graph, const AlgorithmParams& params);

} // namespace petri
