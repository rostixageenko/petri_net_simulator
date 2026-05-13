#include "petri/runtime/algorithm_runtime.hpp"

namespace petri {

Result<AlgorithmResult> run_algorithm(const std::string& name, const DirectedGraph& graph,
                                      const AlgorithmParams& params) {
    if (name == "dfs") {
        return run_dfs(graph, params);
    }
    if (name == "bfs") {
        return run_bfs(graph, params);
    }
    if (name == "dijkstra") {
        return run_dijkstra(graph, params);
    }
    return Result<AlgorithmResult>::failure(
        make_error("UNKNOWN_ALGORITHM", "Unknown algorithm", {{"algorithm", name}}));
}

} // namespace petri
