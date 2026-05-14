#include "petri/graph_models/star_forms.hpp"

#include <unordered_map>
#include <utility>

namespace petri {
namespace {

struct IncomingEdge {
    GraphEdge edge;
    std::size_t source_index = 0;
};

std::unordered_map<std::string, std::size_t> vertex_index_map(const std::vector<std::string>& vertex_ids) {
    std::unordered_map<std::string, std::size_t> index_by_id;
    for (std::size_t i = 0; i < vertex_ids.size(); ++i) {
        index_by_id[vertex_ids[i]] = i;
    }
    return index_by_id;
}

nlohmann::json string_vector_to_json(const std::vector<std::string>& values) {
    nlohmann::json result = nlohmann::json::array();
    for (const auto& value : values) {
        result.push_back(value);
    }
    return result;
}

nlohmann::json size_vector_to_json(const std::vector<std::size_t>& values) {
    nlohmann::json result = nlohmann::json::array();
    for (const auto value : values) {
        result.push_back(value);
    }
    return result;
}

Result<std::string> read_string_field(const nlohmann::json& data, const std::string& field) {
    if (!data.contains(field) || !data.at(field).is_string()) {
        return Result<std::string>::failure(
            make_error("INVALID_JSON", "Graph form field must be a string", {{"field", field}}));
    }
    return Result<std::string>::success(data.at(field).get<std::string>());
}

Result<double> read_double_field(const nlohmann::json& data, const std::string& field) {
    if (!data.contains(field) || !data.at(field).is_number()) {
        return Result<double>::failure(
            make_error("INVALID_JSON", "Graph form field must be a number", {{"field", field}}));
    }
    return Result<double>::success(data.at(field).get<double>());
}

Result<std::size_t> read_size_field(const nlohmann::json& data, const std::string& field) {
    if (!data.contains(field) || !data.at(field).is_number_integer()) {
        return Result<std::size_t>::failure(
            make_error("INVALID_JSON", "Graph form field must be a non-negative integer", {{"field", field}}));
    }
    const auto value = data.at(field).get<int>();
    if (value < 0) {
        return Result<std::size_t>::failure(
            make_error("INVALID_JSON", "Graph form field must be a non-negative integer", {{"field", field}}));
    }
    return Result<std::size_t>::success(static_cast<std::size_t>(value));
}

Result<std::vector<std::string>> read_string_array(const nlohmann::json& data, const std::string& field) {
    if (!data.contains(field) || !data.at(field).is_array()) {
        return Result<std::vector<std::string>>::failure(
            make_error("INVALID_JSON", "Graph form field must be an array", {{"field", field}}));
    }

    std::vector<std::string> values;
    for (const auto& item : data.at(field)) {
        if (!item.is_string()) {
            return Result<std::vector<std::string>>::failure(
                make_error("INVALID_JSON", "Graph form array item must be a string", {{"field", field}}));
        }
        values.push_back(item.get<std::string>());
    }
    return Result<std::vector<std::string>>::success(std::move(values));
}

Result<std::vector<std::size_t>> read_size_array(const nlohmann::json& data, const std::string& field) {
    if (!data.contains(field) || !data.at(field).is_array()) {
        return Result<std::vector<std::size_t>>::failure(
            make_error("INVALID_JSON", "Graph form field must be an array", {{"field", field}}));
    }

    std::vector<std::size_t> values;
    for (const auto& item : data.at(field)) {
        if (!item.is_number_integer()) {
            return Result<std::vector<std::size_t>>::failure(
                make_error("INVALID_JSON", "Graph form array item must be an integer", {{"field", field}}));
        }
        const auto value = item.get<int>();
        if (value < 0) {
            return Result<std::vector<std::size_t>>::failure(
                make_error("INVALID_JSON", "Graph form array item must be non-negative", {{"field", field}}));
        }
        values.push_back(static_cast<std::size_t>(value));
    }
    return Result<std::vector<std::size_t>>::success(std::move(values));
}

Result<void> validate_row_offsets(const std::vector<std::size_t>& offsets, std::size_t vertex_count,
                                  std::size_t edge_count) {
    if (offsets.size() != vertex_count + 1) {
        return Result<void>::failure(
            make_error("INVALID_JSON", "row_offsets size must equal vertex count plus one"));
    }
    if (offsets.empty() || offsets.front() != 0 || offsets.back() != edge_count) {
        return Result<void>::failure(
            make_error("INVALID_JSON", "row_offsets must start at zero and end at edge count"));
    }
    for (std::size_t i = 1; i < offsets.size(); ++i) {
        if (offsets[i] < offsets[i - 1]) {
            return Result<void>::failure(make_error("INVALID_JSON", "row_offsets must be monotonic"));
        }
    }
    return Result<void>::success();
}

Result<void> validate_star_array_sizes(std::size_t edge_count, std::size_t endpoint_count,
                                       std::size_t index_count, std::size_t label_count,
                                       std::size_t weight_count) {
    if (edge_count != endpoint_count || edge_count != index_count ||
        edge_count != label_count || edge_count != weight_count) {
        return Result<void>::failure(make_error("INVALID_JSON", "Star form edge arrays must have equal size"));
    }
    return Result<void>::success();
}

Result<void> validate_vertex_index(const std::unordered_map<std::string, std::size_t>& index_by_id,
                                   const std::string& vertex_id, std::size_t vertex_index,
                                   const std::string& field) {
    const auto it = index_by_id.find(vertex_id);
    if (it == index_by_id.end()) {
        return Result<void>::failure(
            make_error("INVALID_JSON", "Graph form references unknown vertex", {{"field", field}}));
    }
    if (it->second != vertex_index) {
        return Result<void>::failure(
            make_error("INVALID_JSON", "Graph form vertex index does not match vertex id", {{"field", field}}));
    }
    return Result<void>::success();
}

} // namespace

AdjacencyList adjacency_list_from_graph(const DirectedGraph& graph) {
    AdjacencyList adjacency;
    adjacency.vertex_ids = graph.vertex_ids();

    const auto index_by_id = vertex_index_map(adjacency.vertex_ids);
    for (const auto& vertex_id : adjacency.vertex_ids) {
        AdjacencyListRow row;
        row.vertex_id = vertex_id;
        for (const auto& edge : graph.edges_from(vertex_id)) {
            row.edges.push_back(AdjacencyListEdge{
                edge.id,
                edge.target,
                index_by_id.at(edge.target),
                edge.label,
                edge.weight,
            });
        }
        adjacency.rows.push_back(std::move(row));
    }

    return adjacency;
}

ForwardStarForm forward_star_from_graph(const DirectedGraph& graph) {
    ForwardStarForm form;
    form.vertex_ids = graph.vertex_ids();
    form.row_offsets.reserve(form.vertex_ids.size() + 1);
    form.row_offsets.push_back(0);

    const auto index_by_id = vertex_index_map(form.vertex_ids);
    for (const auto& vertex_id : form.vertex_ids) {
        for (const auto& edge : graph.edges_from(vertex_id)) {
            form.edge_ids.push_back(edge.id);
            form.target_vertex_ids.push_back(edge.target);
            form.target_indices.push_back(index_by_id.at(edge.target));
            form.labels.push_back(edge.label);
            form.weights.push_back(edge.weight);
        }
        form.row_offsets.push_back(form.edge_ids.size());
    }

    return form;
}

BackwardStarForm backward_star_from_graph(const DirectedGraph& graph) {
    BackwardStarForm form;
    form.vertex_ids = graph.vertex_ids();
    form.row_offsets.reserve(form.vertex_ids.size() + 1);
    form.row_offsets.push_back(0);

    const auto index_by_id = vertex_index_map(form.vertex_ids);
    std::vector<std::vector<IncomingEdge>> incoming(form.vertex_ids.size());
    for (std::size_t source_index = 0; source_index < form.vertex_ids.size(); ++source_index) {
        const auto& source_id = form.vertex_ids[source_index];
        for (const auto& edge : graph.edges_from(source_id)) {
            incoming[index_by_id.at(edge.target)].push_back(IncomingEdge{edge, source_index});
        }
    }

    for (const auto& incoming_edges : incoming) {
        for (const auto& incoming_edge : incoming_edges) {
            form.edge_ids.push_back(incoming_edge.edge.id);
            form.source_vertex_ids.push_back(incoming_edge.edge.source);
            form.source_indices.push_back(incoming_edge.source_index);
            form.labels.push_back(incoming_edge.edge.label);
            form.weights.push_back(incoming_edge.edge.weight);
        }
        form.row_offsets.push_back(form.edge_ids.size());
    }

    return form;
}

AdjacencyList adjacency_list_from_reachability_graph(const ReachabilityGraph& reachability) {
    return adjacency_list_from_graph(reachability.graph);
}

ForwardStarForm forward_star_from_reachability_graph(const ReachabilityGraph& reachability) {
    return forward_star_from_graph(reachability.graph);
}

BackwardStarForm backward_star_from_reachability_graph(const ReachabilityGraph& reachability) {
    return backward_star_from_graph(reachability.graph);
}

nlohmann::json adjacency_list_to_json(const AdjacencyList& adjacency) {
    nlohmann::json result = nlohmann::json::object();
    result["type"] = "adjacency_list";
    result["vertices"] = nlohmann::json::array();

    for (const auto& row : adjacency.rows) {
        nlohmann::json vertex = nlohmann::json::object();
        vertex["id"] = row.vertex_id;
        vertex["edges"] = nlohmann::json::array();
        for (const auto& edge : row.edges) {
            nlohmann::json edge_json = nlohmann::json::object();
            edge_json["id"] = edge.edge_id;
            edge_json["target"] = edge.target_vertex_id;
            edge_json["target_index"] = edge.target_index;
            edge_json["label"] = edge.label;
            edge_json["weight"] = edge.weight;
            vertex["edges"].push_back(edge_json);
        }
        result["vertices"].push_back(vertex);
    }

    return result;
}

Result<AdjacencyList> adjacency_list_from_json(const nlohmann::json& data) {
    if (!data.is_object() || !data.contains("vertices") || !data.at("vertices").is_array()) {
        return Result<AdjacencyList>::failure(
            make_error("INVALID_JSON", "Adjacency list JSON must contain array field 'vertices'"));
    }

    AdjacencyList adjacency;
    for (const auto& vertex_json : data.at("vertices")) {
        if (!vertex_json.is_object()) {
            return Result<AdjacencyList>::failure(
                make_error("INVALID_JSON", "Adjacency list vertex must be an object"));
        }
        auto vertex_id = read_string_field(vertex_json, "id");
        if (!vertex_id) {
            return Result<AdjacencyList>::failure(vertex_id.error());
        }
        if (!vertex_json.contains("edges") || !vertex_json.at("edges").is_array()) {
            return Result<AdjacencyList>::failure(
                make_error("INVALID_JSON", "Adjacency list vertex must contain array field 'edges'"));
        }

        AdjacencyListRow row;
        row.vertex_id = vertex_id.value();
        adjacency.vertex_ids.push_back(row.vertex_id);
        for (const auto& edge_json : vertex_json.at("edges")) {
            if (!edge_json.is_object()) {
                return Result<AdjacencyList>::failure(
                    make_error("INVALID_JSON", "Adjacency list edge must be an object"));
            }
            auto edge_id = read_string_field(edge_json, "id");
            auto target = read_string_field(edge_json, "target");
            auto target_index = read_size_field(edge_json, "target_index");
            auto label = read_string_field(edge_json, "label");
            auto weight = read_double_field(edge_json, "weight");
            if (!edge_id) {
                return Result<AdjacencyList>::failure(edge_id.error());
            }
            if (!target) {
                return Result<AdjacencyList>::failure(target.error());
            }
            if (!target_index) {
                return Result<AdjacencyList>::failure(target_index.error());
            }
            if (!label) {
                return Result<AdjacencyList>::failure(label.error());
            }
            if (!weight) {
                return Result<AdjacencyList>::failure(weight.error());
            }

            row.edges.push_back(AdjacencyListEdge{
                edge_id.value(),
                target.value(),
                target_index.value(),
                label.value(),
                weight.value(),
            });
        }
        adjacency.rows.push_back(std::move(row));
    }

    const auto index_by_id = vertex_index_map(adjacency.vertex_ids);
    for (const auto& row : adjacency.rows) {
        for (const auto& edge : row.edges) {
            auto validation = validate_vertex_index(index_by_id, edge.target_vertex_id, edge.target_index, "target");
            if (!validation) {
                return Result<AdjacencyList>::failure(validation.error());
            }
        }
    }

    return Result<AdjacencyList>::success(std::move(adjacency));
}

nlohmann::json forward_star_to_json(const ForwardStarForm& form) {
    nlohmann::json result = nlohmann::json::object();
    result["type"] = "fsf";
    result["vertex_ids"] = string_vector_to_json(form.vertex_ids);
    result["row_offsets"] = size_vector_to_json(form.row_offsets);
    result["edges"] = nlohmann::json::array();

    for (std::size_t i = 0; i < form.edge_ids.size(); ++i) {
        nlohmann::json edge = nlohmann::json::object();
        edge["id"] = form.edge_ids[i];
        edge["target"] = form.target_vertex_ids[i];
        edge["target_index"] = form.target_indices[i];
        edge["label"] = form.labels[i];
        edge["weight"] = form.weights[i];
        result["edges"].push_back(edge);
    }

    return result;
}

Result<ForwardStarForm> forward_star_from_json(const nlohmann::json& data) {
    if (!data.is_object() || !data.contains("edges") || !data.at("edges").is_array()) {
        return Result<ForwardStarForm>::failure(
            make_error("INVALID_JSON", "Forward star JSON must contain array field 'edges'"));
    }

    auto vertex_ids = read_string_array(data, "vertex_ids");
    auto row_offsets = read_size_array(data, "row_offsets");
    if (!vertex_ids) {
        return Result<ForwardStarForm>::failure(vertex_ids.error());
    }
    if (!row_offsets) {
        return Result<ForwardStarForm>::failure(row_offsets.error());
    }

    ForwardStarForm form;
    form.vertex_ids = vertex_ids.value();
    form.row_offsets = row_offsets.value();
    for (const auto& edge_json : data.at("edges")) {
        if (!edge_json.is_object()) {
            return Result<ForwardStarForm>::failure(make_error("INVALID_JSON", "Forward star edge must be an object"));
        }
        auto edge_id = read_string_field(edge_json, "id");
        auto target = read_string_field(edge_json, "target");
        auto target_index = read_size_field(edge_json, "target_index");
        auto label = read_string_field(edge_json, "label");
        auto weight = read_double_field(edge_json, "weight");
        if (!edge_id) {
            return Result<ForwardStarForm>::failure(edge_id.error());
        }
        if (!target) {
            return Result<ForwardStarForm>::failure(target.error());
        }
        if (!target_index) {
            return Result<ForwardStarForm>::failure(target_index.error());
        }
        if (!label) {
            return Result<ForwardStarForm>::failure(label.error());
        }
        if (!weight) {
            return Result<ForwardStarForm>::failure(weight.error());
        }
        form.edge_ids.push_back(edge_id.value());
        form.target_vertex_ids.push_back(target.value());
        form.target_indices.push_back(target_index.value());
        form.labels.push_back(label.value());
        form.weights.push_back(weight.value());
    }

    auto size_validation = validate_star_array_sizes(form.edge_ids.size(), form.target_vertex_ids.size(),
                                                    form.target_indices.size(), form.labels.size(), form.weights.size());
    if (!size_validation) {
        return Result<ForwardStarForm>::failure(size_validation.error());
    }
    auto offset_validation = validate_row_offsets(form.row_offsets, form.vertex_ids.size(), form.edge_ids.size());
    if (!offset_validation) {
        return Result<ForwardStarForm>::failure(offset_validation.error());
    }

    const auto index_by_id = vertex_index_map(form.vertex_ids);
    for (std::size_t i = 0; i < form.target_vertex_ids.size(); ++i) {
        auto validation = validate_vertex_index(index_by_id, form.target_vertex_ids[i], form.target_indices[i], "target");
        if (!validation) {
            return Result<ForwardStarForm>::failure(validation.error());
        }
    }

    return Result<ForwardStarForm>::success(std::move(form));
}

nlohmann::json backward_star_to_json(const BackwardStarForm& form) {
    nlohmann::json result = nlohmann::json::object();
    result["type"] = "bsf";
    result["vertex_ids"] = string_vector_to_json(form.vertex_ids);
    result["row_offsets"] = size_vector_to_json(form.row_offsets);
    result["edges"] = nlohmann::json::array();

    for (std::size_t i = 0; i < form.edge_ids.size(); ++i) {
        nlohmann::json edge = nlohmann::json::object();
        edge["id"] = form.edge_ids[i];
        edge["source"] = form.source_vertex_ids[i];
        edge["source_index"] = form.source_indices[i];
        edge["label"] = form.labels[i];
        edge["weight"] = form.weights[i];
        result["edges"].push_back(edge);
    }

    return result;
}

Result<BackwardStarForm> backward_star_from_json(const nlohmann::json& data) {
    if (!data.is_object() || !data.contains("edges") || !data.at("edges").is_array()) {
        return Result<BackwardStarForm>::failure(
            make_error("INVALID_JSON", "Backward star JSON must contain array field 'edges'"));
    }

    auto vertex_ids = read_string_array(data, "vertex_ids");
    auto row_offsets = read_size_array(data, "row_offsets");
    if (!vertex_ids) {
        return Result<BackwardStarForm>::failure(vertex_ids.error());
    }
    if (!row_offsets) {
        return Result<BackwardStarForm>::failure(row_offsets.error());
    }

    BackwardStarForm form;
    form.vertex_ids = vertex_ids.value();
    form.row_offsets = row_offsets.value();
    for (const auto& edge_json : data.at("edges")) {
        if (!edge_json.is_object()) {
            return Result<BackwardStarForm>::failure(
                make_error("INVALID_JSON", "Backward star edge must be an object"));
        }
        auto edge_id = read_string_field(edge_json, "id");
        auto source = read_string_field(edge_json, "source");
        auto source_index = read_size_field(edge_json, "source_index");
        auto label = read_string_field(edge_json, "label");
        auto weight = read_double_field(edge_json, "weight");
        if (!edge_id) {
            return Result<BackwardStarForm>::failure(edge_id.error());
        }
        if (!source) {
            return Result<BackwardStarForm>::failure(source.error());
        }
        if (!source_index) {
            return Result<BackwardStarForm>::failure(source_index.error());
        }
        if (!label) {
            return Result<BackwardStarForm>::failure(label.error());
        }
        if (!weight) {
            return Result<BackwardStarForm>::failure(weight.error());
        }
        form.edge_ids.push_back(edge_id.value());
        form.source_vertex_ids.push_back(source.value());
        form.source_indices.push_back(source_index.value());
        form.labels.push_back(label.value());
        form.weights.push_back(weight.value());
    }

    auto size_validation = validate_star_array_sizes(form.edge_ids.size(), form.source_vertex_ids.size(),
                                                    form.source_indices.size(), form.labels.size(), form.weights.size());
    if (!size_validation) {
        return Result<BackwardStarForm>::failure(size_validation.error());
    }
    auto offset_validation = validate_row_offsets(form.row_offsets, form.vertex_ids.size(), form.edge_ids.size());
    if (!offset_validation) {
        return Result<BackwardStarForm>::failure(offset_validation.error());
    }

    const auto index_by_id = vertex_index_map(form.vertex_ids);
    for (std::size_t i = 0; i < form.source_vertex_ids.size(); ++i) {
        auto validation = validate_vertex_index(index_by_id, form.source_vertex_ids[i], form.source_indices[i], "source");
        if (!validation) {
            return Result<BackwardStarForm>::failure(validation.error());
        }
    }

    return Result<BackwardStarForm>::success(std::move(form));
}

} // namespace petri
