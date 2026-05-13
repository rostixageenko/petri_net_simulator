#include "petri/graph_models/directed_graph.hpp"

namespace petri {

void DirectedGraph::add_vertex(const std::string& id) {
    if (has_vertex(id)) {
        return;
    }
    index_by_id_[id] = vertex_ids_.size();
    vertex_ids_.push_back(id);
    adjacency_.push_back({});
}

void DirectedGraph::add_edge(GraphEdge edge) {
    add_vertex(edge.source);
    add_vertex(edge.target);
    const auto source_index = index_by_id_.at(edge.source);
    adjacency_[source_index].push_back(std::move(edge));
    ++edge_count_;
}

bool DirectedGraph::has_vertex(const std::string& id) const {
    return index_by_id_.find(id) != index_by_id_.end();
}

const std::vector<GraphEdge>& DirectedGraph::edges_from(const std::string& id) const {
    static const std::vector<GraphEdge> empty;
    const auto it = index_by_id_.find(id);
    if (it == index_by_id_.end()) {
        return empty;
    }
    return adjacency_[it->second];
}

std::optional<GraphEdge> DirectedGraph::edge_between(const std::string& source, const std::string& target) const {
    for (const auto& edge : edges_from(source)) {
        if (edge.target == target) {
            return edge;
        }
    }
    return std::nullopt;
}

} // namespace petri
