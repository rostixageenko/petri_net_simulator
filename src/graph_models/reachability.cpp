#include "petri/graph_models/reachability.hpp"

#include <queue>
#include <unordered_map>

namespace petri {

Result<ReachabilityGraph> build_reachability_graph(const PetriNet& net, const ReachabilityOptions& options) {
    if (options.max_states == 0) {
        return Result<ReachabilityGraph>::failure(
            make_error("STATE_LIMIT_EXCEEDED", "max_states must be greater than zero"));
    }

    ReachabilityGraph reachability;
    std::unordered_map<std::string, std::string> vertex_by_marking_key;
    std::queue<std::string> queue;
    std::size_t next_vertex_index = 0;
    std::size_t next_edge_index = 0;

    auto add_state = [&](const Marking& marking, std::size_t depth) -> Result<std::string> {
        const auto key = marking.key();
        const auto existing = vertex_by_marking_key.find(key);
        if (existing != vertex_by_marking_key.end()) {
            return Result<std::string>::success(existing->second);
        }
        if (reachability.graph.vertex_count() >= options.max_states) {
            return Result<std::string>::failure(
                make_error("STATE_LIMIT_EXCEEDED", "Reachability graph state limit exceeded"));
        }

        const std::string vertex_id = "m" + std::to_string(next_vertex_index++);
        vertex_by_marking_key[key] = vertex_id;
        reachability.graph.add_vertex(vertex_id);
        reachability.markings[vertex_id] = marking;
        reachability.depths[vertex_id] = depth;
        queue.push(vertex_id);
        return Result<std::string>::success(vertex_id);
    };

    auto initial = add_state(net.initial_marking(), 0);
    if (!initial) {
        return Result<ReachabilityGraph>::failure(initial.error());
    }
    reachability.initial_vertex_id = initial.value();

    while (!queue.empty()) {
        const std::string current_vertex = queue.front();
        queue.pop();

        const auto current_depth = reachability.depths.at(current_vertex);
        if (current_depth >= options.max_depth) {
            continue;
        }

        const auto current_marking = reachability.markings.at(current_vertex);
        for (const auto& transition_id : net.enabled_transitions(current_marking)) {
            auto next_marking = net.fire_transition(current_marking, transition_id);
            if (!next_marking) {
                return Result<ReachabilityGraph>::failure(next_marking.error());
            }

            auto target_vertex = add_state(next_marking.value(), current_depth + 1);
            if (!target_vertex) {
                return Result<ReachabilityGraph>::failure(target_vertex.error());
            }

            reachability.graph.add_edge(GraphEdge{
                "e" + std::to_string(next_edge_index++),
                current_vertex,
                target_vertex.value(),
                transition_id,
                net.transition_fire_time(transition_id),
            });
        }
    }

    return Result<ReachabilityGraph>::success(std::move(reachability));
}

Result<std::string> find_marking_vertex(const PetriNet& net, const ReachabilityGraph& reachability,
                                        const nlohmann::json& partial_marking) {
    for (const auto& [vertex_id, marking] : reachability.markings) {
        if (net.marking_matches(marking, partial_marking)) {
            return Result<std::string>::success(vertex_id);
        }
    }
    return Result<std::string>::failure(
        make_error("TARGET_NOT_FOUND", "Target marking was not found in the reachability graph"));
}

nlohmann::json reachability_graph_to_json(const PetriNet& net, const ReachabilityGraph& reachability) {
    nlohmann::json result = nlohmann::json::object();
    result["initial_vertex_id"] = reachability.initial_vertex_id;
    result["vertices"] = nlohmann::json::array();
    result["edges"] = nlohmann::json::array();

    for (const auto& vertex_id : reachability.graph.vertex_ids()) {
        nlohmann::json vertex = nlohmann::json::object();
        vertex["id"] = vertex_id;
        vertex["depth"] = reachability.depths.at(vertex_id);
        vertex["marking"] = net.marking_to_json(reachability.markings.at(vertex_id));
        result["vertices"].push_back(vertex);

        for (const auto& edge : reachability.graph.edges_from(vertex_id)) {
            nlohmann::json edge_json = nlohmann::json::object();
            edge_json["id"] = edge.id;
            edge_json["source"] = edge.source;
            edge_json["target"] = edge.target;
            edge_json["transition"] = edge.label;
            edge_json["weight"] = edge.weight;
            result["edges"].push_back(edge_json);
        }
    }

    return result;
}

} // namespace petri
