#pragma once

#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace petri {

struct GraphEdge {
    std::string id;
    std::string source;
    std::string target;
    std::string label;
    double weight = 1.0;
};

class DirectedGraph {
public:
    void add_vertex(const std::string& id);
    void add_edge(GraphEdge edge);

    bool has_vertex(const std::string& id) const;
    const std::vector<std::string>& vertex_ids() const {
        return vertex_ids_;
    }

    const std::vector<GraphEdge>& edges_from(const std::string& id) const;
    std::optional<GraphEdge> edge_between(const std::string& source, const std::string& target) const;

    std::size_t vertex_count() const {
        return vertex_ids_.size();
    }

    std::size_t edge_count() const {
        return edge_count_;
    }

private:
    std::unordered_map<std::string, std::size_t> index_by_id_;
    std::vector<std::string> vertex_ids_;
    std::vector<std::vector<GraphEdge>> adjacency_;
    std::size_t edge_count_ = 0;
};

} // namespace petri
