#include "petri/algorithms/algorithms.hpp"

#include <algorithm>
#include <chrono>
#include <limits>
#include <queue>
#include <stack>
#include <unordered_map>
#include <unordered_set>

namespace petri {
namespace {

using Clock = std::chrono::steady_clock;

double elapsed_ms(Clock::time_point start) {
    return std::chrono::duration<double, std::milli>(Clock::now() - start).count();
}

std::vector<std::string> reconstruct_path(const std::string& source, const std::string& target,
                                          const std::unordered_map<std::string, std::string>& parent) {
    std::vector<std::string> reversed;
    std::string current = target;
    reversed.push_back(current);
    while (current != source) {
        const auto it = parent.find(current);
        if (it == parent.end()) {
            return {};
        }
        current = it->second;
        reversed.push_back(current);
    }
    std::reverse(reversed.begin(), reversed.end());
    return reversed;
}

double path_cost(const DirectedGraph& graph, const std::vector<std::string>& path) {
    double cost = 0.0;
    for (std::size_t i = 1; i < path.size(); ++i) {
        const auto edge = graph.edge_between(path[i - 1], path[i]);
        if (edge) {
            cost += edge->weight;
        }
    }
    return cost;
}

AlgorithmResult make_traversal_result(const std::string& algorithm, const DirectedGraph& graph,
                                      const AlgorithmParams& params,
                                      const std::unordered_map<std::string, std::string>& parent,
                                      std::vector<std::string> traversal_order,
                                      AlgorithmMetrics metrics, bool target_found,
                                      Clock::time_point start) {
    AlgorithmResult result;
    result.algorithm = algorithm;
    result.status = target_found ? "ok" : "target_not_found";
    result.found = target_found;

    if (!params.target_vertex_id) {
        result.status = "ok";
        result.found = true;
        result.path = std::move(traversal_order);
    } else if (target_found) {
        result.path = reconstruct_path(params.source_vertex_id, *params.target_vertex_id, parent);
    }

    metrics.path_length = result.path.empty() ? 0 : result.path.size() - 1;
    metrics.path_cost = path_cost(graph, result.path);
    metrics.elapsed_ms = elapsed_ms(start);
    result.metrics = metrics;
    return result;
}

Result<void> validate_source(const DirectedGraph& graph, const AlgorithmParams& params) {
    if (!graph.has_vertex(params.source_vertex_id)) {
        return Result<void>::failure(make_error(
            "UNKNOWN_NODE_ID", "Source vertex does not exist", {{"source", params.source_vertex_id}}));
    }
    if (params.target_vertex_id && !graph.has_vertex(*params.target_vertex_id)) {
        return Result<void>::failure(make_error(
            "TARGET_NOT_FOUND", "Target vertex does not exist", {{"target", *params.target_vertex_id}}));
    }
    return Result<void>::success();
}

} // namespace

Result<AlgorithmResult> run_bfs(const DirectedGraph& graph, const AlgorithmParams& params) {
    auto validation = validate_source(graph, params);
    if (!validation) {
        return Result<AlgorithmResult>::failure(validation.error());
    }

    const auto start = Clock::now();
    AlgorithmMetrics metrics;
    std::queue<std::string> queue;
    std::unordered_set<std::string> visited;
    std::unordered_map<std::string, std::string> parent;
    std::vector<std::string> traversal_order;

    queue.push(params.source_vertex_id);
    visited.insert(params.source_vertex_id);

    bool target_found = !params.target_vertex_id || params.source_vertex_id == *params.target_vertex_id;
    while (!queue.empty() && !target_found) {
        const std::string current = queue.front();
        queue.pop();
        ++metrics.visited_vertices;
        traversal_order.push_back(current);

        for (const auto& edge : graph.edges_from(current)) {
            ++metrics.scanned_edges;
            if (visited.insert(edge.target).second) {
                parent[edge.target] = current;
                if (params.target_vertex_id && edge.target == *params.target_vertex_id) {
                    target_found = true;
                    break;
                }
                queue.push(edge.target);
            }
        }
    }

    if (target_found && params.target_vertex_id) {
        traversal_order.push_back(*params.target_vertex_id);
    }

    return Result<AlgorithmResult>::success(make_traversal_result(
        "bfs", graph, params, parent, std::move(traversal_order), metrics, target_found, start));
}

Result<AlgorithmResult> run_dfs(const DirectedGraph& graph, const AlgorithmParams& params) {
    auto validation = validate_source(graph, params);
    if (!validation) {
        return Result<AlgorithmResult>::failure(validation.error());
    }

    const auto start = Clock::now();
    AlgorithmMetrics metrics;
    std::stack<std::string> stack;
    std::unordered_set<std::string> visited;
    std::unordered_map<std::string, std::string> parent;
    std::vector<std::string> traversal_order;

    stack.push(params.source_vertex_id);
    bool target_found = false;

    while (!stack.empty()) {
        const std::string current = stack.top();
        stack.pop();
        if (!visited.insert(current).second) {
            continue;
        }

        ++metrics.visited_vertices;
        traversal_order.push_back(current);
        if (params.target_vertex_id && current == *params.target_vertex_id) {
            target_found = true;
            break;
        }

        const auto& edges = graph.edges_from(current);
        metrics.scanned_edges += edges.size();
        for (auto it = edges.rbegin(); it != edges.rend(); ++it) {
            if (visited.find(it->target) == visited.end()) {
                if (parent.find(it->target) == parent.end()) {
                    parent[it->target] = current;
                }
                stack.push(it->target);
            }
        }
    }

    if (!params.target_vertex_id) {
        target_found = true;
    }

    return Result<AlgorithmResult>::success(make_traversal_result(
        "dfs", graph, params, parent, std::move(traversal_order), metrics, target_found, start));
}

Result<AlgorithmResult> run_dijkstra(const DirectedGraph& graph, const AlgorithmParams& params) {
    auto validation = validate_source(graph, params);
    if (!validation) {
        return Result<AlgorithmResult>::failure(validation.error());
    }

    const auto start = Clock::now();
    AlgorithmMetrics metrics;
    using QueueItem = std::pair<double, std::string>;
    std::priority_queue<QueueItem, std::vector<QueueItem>, std::greater<>> queue;
    std::unordered_map<std::string, double> distance;
    std::unordered_map<std::string, std::string> parent;
    std::unordered_set<std::string> settled;
    std::vector<std::string> traversal_order;

    for (const auto& vertex : graph.vertex_ids()) {
        distance[vertex] = std::numeric_limits<double>::infinity();
    }
    distance[params.source_vertex_id] = 0.0;
    queue.push({0.0, params.source_vertex_id});

    bool target_found = false;
    while (!queue.empty()) {
        const auto [current_distance, current] = queue.top();
        queue.pop();
        if (current_distance > distance[current] || !settled.insert(current).second) {
            continue;
        }

        ++metrics.visited_vertices;
        traversal_order.push_back(current);
        if (params.target_vertex_id && current == *params.target_vertex_id) {
            target_found = true;
            break;
        }

        for (const auto& edge : graph.edges_from(current)) {
            if (edge.weight < 0.0) {
                return Result<AlgorithmResult>::failure(make_error(
                    "NEGATIVE_EDGE_WEIGHT", "Dijkstra cannot process negative edge weights", {{"edge_id", edge.id}}));
            }
            ++metrics.scanned_edges;
            const double candidate = current_distance + edge.weight;
            if (candidate < distance[edge.target]) {
                distance[edge.target] = candidate;
                parent[edge.target] = current;
                queue.push({candidate, edge.target});
            }
        }
    }

    if (!params.target_vertex_id) {
        target_found = true;
    }

    AlgorithmResult result = make_traversal_result(
        "dijkstra", graph, params, parent, std::move(traversal_order), metrics, target_found, start);
    if (target_found && params.target_vertex_id) {
        result.metrics.path_cost = distance[*params.target_vertex_id];
    }
    return Result<AlgorithmResult>::success(std::move(result));
}

} // namespace petri
