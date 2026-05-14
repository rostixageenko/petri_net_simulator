#include "petri/service_adapter/json_api.hpp"

#include "petri/logging_metrics/metrics_logger.hpp"

#include <exception>
#include <optional>
#include <string>
#include <utility>

namespace petri {
namespace {

std::string request_id_from(const nlohmann::json& request) {
    if (request.is_object() && request.contains("request_id") && request.at("request_id").is_string()) {
        return request.at("request_id").get<std::string>();
    }
    return "";
}

struct NetCounts {
    std::size_t places = 0;
    std::size_t transitions = 0;
    std::size_t arcs = 0;
};

std::size_t array_field_size(const nlohmann::json& object, const std::string& field) {
    if (object.is_object() && object.contains(field) && object.at(field).is_array()) {
        return object.at(field).size();
    }
    return 0;
}

NetCounts net_counts_from_request(const nlohmann::json& request) {
    if (!request.is_object() || !request.contains("net") || !request.at("net").is_object()) {
        return {};
    }
    const auto& net = request.at("net");
    return NetCounts{
        array_field_size(net, "places"),
        array_field_size(net, "transitions"),
        array_field_size(net, "arcs"),
    };
}

NetCounts net_counts_from_net(const PetriNet& net) {
    return NetCounts{net.places().size(), net.transitions().size(), net.arcs().size()};
}

std::string algorithm_from_request(const nlohmann::json& request) {
    if (request.is_object() && request.contains("params") && request.at("params").is_object() &&
        request.at("params").contains("algorithm") && request.at("params").at("algorithm").is_string()) {
        return request.at("params").at("algorithm").get<std::string>();
    }
    return "bfs";
}

std::string graph_mode_from_request(const nlohmann::json& request) {
    if (request.is_object() && request.contains("params") && request.at("params").is_object() &&
        request.at("params").contains("graph_mode") && request.at("params").at("graph_mode").is_string()) {
        return request.at("params").at("graph_mode").get<std::string>();
    }
    return "reachability_graph";
}

void log_metrics_record(const std::string& request_id, const std::string& algorithm_name,
                        const std::string& task_type, const NetCounts& counts,
                        std::size_t built_state_count, std::size_t scanned_edge_count,
                        double elapsed_ms, bool found, double path_cost, std::size_t path_length,
                        const std::string& error_message) {
    const auto record = create_metrics_record(RunMetricsInput{
        request_id,
        algorithm_name,
        task_type,
        counts.places,
        counts.transitions,
        counts.arcs,
        built_state_count,
        scanned_edge_count,
        elapsed_ms,
        found,
        path_cost,
        path_length,
        error_message,
    });
    (void)append_metrics_record(record);
}

nlohmann::json event_to_json(const PetriNet& net, const SimulationEvent& event) {
    nlohmann::json result = nlohmann::json::object();
    result["step"] = event.step;
    result["time"] = event.time;
    result["fired_transition"] = event.fired_transition;
    result["marking_before"] = net.marking_to_json(event.marking_before);
    result["marking_after"] = net.marking_to_json(event.marking_after);
    result["enabled_before"] = nlohmann::json::array();
    result["enabled_after"] = nlohmann::json::array();
    for (const auto& transition_id : event.enabled_before) {
        result["enabled_before"].push_back(transition_id);
    }
    for (const auto& transition_id : event.enabled_after) {
        result["enabled_after"].push_back(transition_id);
    }
    return result;
}

nlohmann::json algorithm_metrics_to_json(const AlgorithmMetrics& metrics) {
    nlohmann::json result = nlohmann::json::object();
    result["elapsed_ms"] = metrics.elapsed_ms;
    result["visited_vertices"] = metrics.visited_vertices;
    result["scanned_edges"] = metrics.scanned_edges;
    result["path_length"] = metrics.path_length;
    result["path_cost"] = metrics.path_cost;
    return result;
}

bool is_non_negative_integer(const nlohmann::json& value) {
    return value.is_number_integer() && value.get<int>() >= 0;
}

Result<nlohmann::json> params_from_request(const nlohmann::json& request) {
    if (!request.is_object()) {
        return Result<nlohmann::json>::failure(make_error("INVALID_JSON", "Request must be a JSON object"));
    }
    if (!request.contains("params")) {
        return Result<nlohmann::json>::success(nlohmann::json::object());
    }
    if (!request.at("params").is_object()) {
        return Result<nlohmann::json>::failure(
            make_error("INVALID_JSON", "Field 'params' must be an object", {{"field", "params"}}));
    }
    return Result<nlohmann::json>::success(request.at("params"));
}

Result<std::size_t> read_size_t_field(const nlohmann::json& object, const std::string& field,
                                      std::size_t default_value) {
    if (!object.contains(field)) {
        return Result<std::size_t>::success(default_value);
    }
    if (!is_non_negative_integer(object.at(field))) {
        return Result<std::size_t>::failure(
            make_error("INVALID_JSON", "Expected non-negative integer field '" + field + "'", {{"field", field}}));
    }
    return Result<std::size_t>::success(object.at(field).get<std::size_t>());
}

Result<bool> read_bool_field(const nlohmann::json& object, const std::string& field, bool default_value) {
    if (!object.contains(field)) {
        return Result<bool>::success(default_value);
    }
    if (!object.at(field).is_boolean()) {
        return Result<bool>::failure(
            make_error("INVALID_JSON", "Expected boolean field '" + field + "'", {{"field", field}}));
    }
    return Result<bool>::success(object.at(field).get<bool>());
}

Result<std::string> read_string_field(const nlohmann::json& object, const std::string& field,
                                      const std::string& default_value) {
    if (!object.contains(field)) {
        return Result<std::string>::success(default_value);
    }
    if (!object.at(field).is_string()) {
        return Result<std::string>::failure(
            make_error("INVALID_JSON", "Expected string field '" + field + "'", {{"field", field}}));
    }
    return Result<std::string>::success(object.at(field).get<std::string>());
}

Result<std::optional<std::string>> read_optional_string_field(const nlohmann::json& object,
                                                              const std::string& field) {
    if (!object.contains(field)) {
        return Result<std::optional<std::string>>::success(std::nullopt);
    }
    if (!object.at(field).is_string()) {
        return Result<std::optional<std::string>>::failure(
            make_error("INVALID_JSON", "Expected string field '" + field + "'", {{"field", field}}));
    }
    return Result<std::optional<std::string>>::success(object.at(field).get<std::string>());
}

Result<void> validate_target_marking(const PetriNet& net, const nlohmann::json& target_marking) {
    if (!target_marking.is_object()) {
        return Result<void>::failure(
            make_error("INVALID_JSON", "Field 'target_marking' must be an object", {{"field", "target_marking"}}));
    }
    if (target_marking.size() == 0) {
        return Result<void>::failure(make_error(
            "INVALID_JSON", "Field 'target_marking' must contain at least one place id",
            {{"field", "target_marking"}}));
    }

    std::size_t known_place_count = 0;
    for (const auto& place : net.places()) {
        if (!target_marking.contains(place.id)) {
            continue;
        }
        ++known_place_count;
        if (!is_non_negative_integer(target_marking.at(place.id))) {
            return Result<void>::failure(make_error(
                "INVALID_TOKEN_COUNT", "Target marking values must be non-negative integers",
                {{"place_id", place.id}}));
        }
    }

    if (known_place_count != target_marking.size()) {
        return Result<void>::failure(make_error(
            "UNKNOWN_NODE_ID", "Target marking references an unknown place id",
            {{"field", "target_marking"}}));
    }
    return Result<void>::success();
}

nlohmann::json path_to_json(const PetriNet& net, const ReachabilityGraph& reachability,
                            const std::vector<std::string>& path) {
    nlohmann::json result = nlohmann::json::array();
    for (std::size_t i = 0; i < path.size(); ++i) {
        nlohmann::json vertex = nlohmann::json::object();
        vertex["vertex_id"] = path[i];
        vertex["marking"] = net.marking_to_json(reachability.markings.at(path[i]));
        if (i > 0) {
            const auto edge = reachability.graph.edge_between(path[i - 1], path[i]);
            if (edge) {
                vertex["via_transition"] = edge->label;
            }
        }
        result.push_back(vertex);
    }
    return result;
}

Result<PetriNet> net_from_request(const nlohmann::json& request) {
    if (!request.is_object() || !request.contains("net")) {
        return Result<PetriNet>::failure(make_error("INVALID_JSON", "Request must contain field 'net'"));
    }
    try {
        return PetriNet::from_json(request.at("net"));
    } catch (const std::exception& ex) {
        return Result<PetriNet>::failure(make_error("INVALID_JSON", ex.what(), {{"field", "net"}}));
    }
}

} // namespace

nlohmann::json error_response(const std::string& request_id, const Error& error) {
    nlohmann::json result = nlohmann::json::object();
    result["request_id"] = request_id;
    result["status"] = "error";
    result["error"]["code"] = error.code;
    result["error"]["message"] = error.message;
    result["error"]["details"] = nlohmann::json::object();
    for (const auto& [key, value] : error.details) {
        result["error"]["details"][key] = value;
    }
    return result;
}

nlohmann::json simulate_request(const nlohmann::json& request) {
    const std::string request_id = request_id_from(request);
    try {
        auto net_result = net_from_request(request);
        if (!net_result) {
            log_metrics_record(request_id, "simulation", "simulation", net_counts_from_request(request),
                               0, 0, 0.0, false, 0.0, 0, net_result.error().message);
            return error_response(request_id, net_result.error());
        }
        const PetriNet& net = net_result.value();
        const NetCounts counts = net_counts_from_net(net);

        auto params_result = params_from_request(request);
        if (!params_result) {
            log_metrics_record(request_id, "simulation", "simulation", counts,
                               0, 0, 0.0, false, 0.0, 0, params_result.error().message);
            return error_response(request_id, params_result.error());
        }
        const nlohmann::json params_json = params_result.value();

        SimulationParams params;
        auto max_steps = read_size_t_field(params_json, "max_steps", params.max_steps);
        if (!max_steps) {
            log_metrics_record(request_id, "simulation", "simulation", counts,
                               0, 0, 0.0, false, 0.0, 0, max_steps.error().message);
            return error_response(request_id, max_steps.error());
        }
        params.max_steps = max_steps.value();

        auto strategy_value = read_optional_string_field(params_json, "strategy");
        if (!strategy_value) {
            log_metrics_record(request_id, "simulation", "simulation", counts,
                               0, 0, 0.0, false, 0.0, 0, strategy_value.error().message);
            return error_response(request_id, strategy_value.error());
        }
        if (strategy_value.value()) {
            auto strategy = parse_strategy(*strategy_value.value());
            if (!strategy) {
                log_metrics_record(request_id, "simulation", "simulation", counts,
                                   0, 0, 0.0, false, 0.0, 0, strategy.error().message);
                return error_response(request_id, strategy.error());
            }
            params.strategy = strategy.value();
        }

        auto stop_on_deadlock = read_bool_field(params_json, "stop_on_deadlock", params.stop_on_deadlock);
        if (!stop_on_deadlock) {
            log_metrics_record(request_id, "simulation", "simulation", counts,
                               0, 0, 0.0, false, 0.0, 0, stop_on_deadlock.error().message);
            return error_response(request_id, stop_on_deadlock.error());
        }
        params.stop_on_deadlock = stop_on_deadlock.value();

        auto return_events = read_bool_field(params_json, "return_events", params.return_events);
        if (!return_events) {
            log_metrics_record(request_id, "simulation", "simulation", counts,
                               0, 0, 0.0, false, 0.0, 0, return_events.error().message);
            return error_response(request_id, return_events.error());
        }
        params.return_events = return_events.value();

        auto transition_id = read_optional_string_field(params_json, "transition_id");
        if (!transition_id) {
            log_metrics_record(request_id, "simulation", "simulation", counts,
                               0, 0, 0.0, false, 0.0, 0, transition_id.error().message);
            return error_response(request_id, transition_id.error());
        }
        params.transition_id = transition_id.value();

        Interpreter interpreter(net);
        auto run_result = interpreter.run(params);
        if (!run_result) {
            log_metrics_record(request_id, "simulation", "simulation", counts,
                               0, 0, 0.0, false, interpreter.current_time(), 0,
                               run_result.error().message);
            return error_response(request_id, run_result.error());
        }

        const auto& run = run_result.value();
        nlohmann::json result = nlohmann::json::object();
        result["request_id"] = request_id;
        result["status"] = "ok";
        result["model_name"] = net.name();
        result["initial_marking"] = net.marking_to_json(run.initial_marking);
        result["final_marking"] = net.marking_to_json(run.final_marking);
        result["events"] = nlohmann::json::array();
        for (const auto& event : run.events) {
            result["events"].push_back(event_to_json(net, event));
        }
        result["metrics"]["elapsed_ms"] = run.elapsed_ms;
        result["metrics"]["steps_executed"] = run.steps_executed;
        result["metrics"]["deadlock"] = run.deadlock;
        log_metrics_record(request_id, "simulation", "simulation", counts,
                           run.steps_executed + 1, 0, run.elapsed_ms, true,
                           interpreter.current_time(), run.steps_executed, "");
        return result;
    } catch (const std::exception& ex) {
        log_metrics_record(request_id, "simulation", "simulation", net_counts_from_request(request),
                           0, 0, 0.0, false, 0.0, 0, ex.what());
        return error_response(request_id, make_error("INVALID_JSON", ex.what()));
    }
}

nlohmann::json algorithm_request(const nlohmann::json& request) {
    const std::string request_id = request_id_from(request);
    const std::string requested_algorithm = algorithm_from_request(request);
    const std::string requested_graph_mode = graph_mode_from_request(request);
    try {
        auto net_result = net_from_request(request);
        if (!net_result) {
            log_metrics_record(request_id, requested_algorithm, requested_graph_mode,
                               net_counts_from_request(request), 0, 0, 0.0, false, 0.0, 0,
                               net_result.error().message);
            return error_response(request_id, net_result.error());
        }
        const PetriNet& net = net_result.value();
        const NetCounts counts = net_counts_from_net(net);

        auto params_result = params_from_request(request);
        if (!params_result) {
            log_metrics_record(request_id, requested_algorithm, requested_graph_mode, counts,
                               0, 0, 0.0, false, 0.0, 0, params_result.error().message);
            return error_response(request_id, params_result.error());
        }
        const nlohmann::json params_json = params_result.value();

        auto graph_mode_result = read_string_field(params_json, "graph_mode", "reachability_graph");
        if (!graph_mode_result) {
            log_metrics_record(request_id, requested_algorithm, requested_graph_mode, counts,
                               0, 0, 0.0, false, 0.0, 0, graph_mode_result.error().message);
            return error_response(request_id, graph_mode_result.error());
        }
        const std::string graph_mode = graph_mode_result.value();
        if (graph_mode != "reachability_graph") {
            const auto error = make_error("INVALID_JSON", "Only reachability_graph is supported in this slice",
                                          {{"graph_mode", graph_mode}});
            log_metrics_record(request_id, requested_algorithm, graph_mode, counts,
                               0, 0, 0.0, false, 0.0, 0, error.message);
            return error_response(request_id, error);
        }

        ReachabilityOptions options;
        auto max_states = read_size_t_field(params_json, "max_states", options.max_states);
        if (!max_states) {
            log_metrics_record(request_id, requested_algorithm, graph_mode, counts,
                               0, 0, 0.0, false, 0.0, 0, max_states.error().message);
            return error_response(request_id, max_states.error());
        }
        options.max_states = max_states.value();

        auto max_depth = read_size_t_field(params_json, "max_depth", options.max_depth);
        if (!max_depth) {
            log_metrics_record(request_id, requested_algorithm, graph_mode, counts,
                               0, 0, 0.0, false, 0.0, 0, max_depth.error().message);
            return error_response(request_id, max_depth.error());
        }
        options.max_depth = max_depth.value();

        if (params_json.contains("target_marking")) {
            auto validation = validate_target_marking(net, params_json.at("target_marking"));
            if (!validation) {
                log_metrics_record(request_id, requested_algorithm, graph_mode, counts,
                                   0, 0, 0.0, false, 0.0, 0, validation.error().message);
                return error_response(request_id, validation.error());
            }
        }

        auto reachability = build_reachability_graph(net, options);
        if (!reachability) {
            log_metrics_record(request_id, requested_algorithm, graph_mode, counts,
                               0, 0, 0.0, false, 0.0, 0, reachability.error().message);
            return error_response(request_id, reachability.error());
        }
        const std::size_t built_states = reachability.value().graph.vertex_count();

        AlgorithmParams algorithm_params;
        auto source_result = read_string_field(params_json, "source", "initial");
        if (!source_result) {
            log_metrics_record(request_id, requested_algorithm, graph_mode, counts,
                               built_states, 0, 0.0, false, 0.0, 0, source_result.error().message);
            return error_response(request_id, source_result.error());
        }
        const std::string source = source_result.value();
        algorithm_params.source_vertex_id = source == "initial" ? reachability.value().initial_vertex_id : source;

        auto target_result = read_optional_string_field(params_json, "target");
        if (!target_result) {
            log_metrics_record(request_id, requested_algorithm, graph_mode, counts,
                               built_states, 0, 0.0, false, 0.0, 0, target_result.error().message);
            return error_response(request_id, target_result.error());
        }
        if (target_result.value()) {
            algorithm_params.target_vertex_id = target_result.value();
        } else if (params_json.contains("target_marking")) {
            auto target = find_marking_vertex(net, reachability.value(), params_json.at("target_marking"));
            if (!target) {
                log_metrics_record(request_id, requested_algorithm, graph_mode, counts,
                                   built_states, 0, 0.0, false, 0.0, 0, target.error().message);
                return error_response(request_id, target.error());
            }
            algorithm_params.target_vertex_id = target.value();
        }

        auto algorithm_name = read_string_field(params_json, "algorithm", "bfs");
        if (!algorithm_name) {
            log_metrics_record(request_id, requested_algorithm, graph_mode, counts,
                               built_states, 0, 0.0, false, 0.0, 0, algorithm_name.error().message);
            return error_response(request_id, algorithm_name.error());
        }
        const std::string algorithm = algorithm_name.value();

        auto algorithm_result = run_algorithm(algorithm, reachability.value().graph, algorithm_params);
        if (!algorithm_result) {
            log_metrics_record(request_id, algorithm, graph_mode, counts,
                               built_states, 0, 0.0, false, 0.0, 0, algorithm_result.error().message);
            return error_response(request_id, algorithm_result.error());
        }

        const auto& result_value = algorithm_result.value();
        if (!result_value.found && algorithm_params.target_vertex_id) {
            const auto error = make_error(
                "TARGET_NOT_FOUND", "Target is not reachable from source",
                {{"source", algorithm_params.source_vertex_id},
                 {"target", *algorithm_params.target_vertex_id},
                 {"algorithm", algorithm},
                 {"visited_vertices", std::to_string(result_value.metrics.visited_vertices)},
                 {"scanned_edges", std::to_string(result_value.metrics.scanned_edges)}});
            log_metrics_record(request_id, algorithm, graph_mode, counts,
                               built_states, result_value.metrics.scanned_edges, result_value.metrics.elapsed_ms,
                               false, result_value.metrics.path_cost, result_value.metrics.path_length, error.message);
            return error_response(request_id, error);
        }

        nlohmann::json result = nlohmann::json::object();
        result["request_id"] = request_id;
        result["status"] = "ok";
        result["algorithm"] = result_value.algorithm;
        result["graph_mode"] = graph_mode;
        result["found"] = result_value.found;
        result["path"] = path_to_json(net, reachability.value(), result_value.path);
        result["metrics"] = algorithm_metrics_to_json(result_value.metrics);
        result["graph"]["vertices"] = reachability.value().graph.vertex_count();
        result["graph"]["edges"] = reachability.value().graph.edge_count();
        log_metrics_record(request_id, result_value.algorithm, graph_mode, counts,
                           built_states, result_value.metrics.scanned_edges, result_value.metrics.elapsed_ms,
                           result_value.found, result_value.metrics.path_cost, result_value.metrics.path_length, "");
        return result;
    } catch (const std::exception& ex) {
        log_metrics_record(request_id, requested_algorithm, requested_graph_mode,
                           net_counts_from_request(request), 0, 0, 0.0, false, 0.0, 0, ex.what());
        return error_response(request_id, make_error("INVALID_JSON", ex.what()));
    }
}

nlohmann::json JsonServiceAdapter::handle(const nlohmann::json& request) const {
    const std::string request_id = request_id_from(request);
    try {
        if (!request.is_object() || !request.contains("mode") || !request.at("mode").is_string()) {
            return error_response(request_id, make_error("INVALID_JSON", "Request must contain string field 'mode'"));
        }

        const std::string mode = request.at("mode").get<std::string>();
        if (mode == "simulate") {
            return simulate_request(request);
        }
        if (mode == "algorithm") {
            return algorithm_request(request);
        }
        return error_response(request_id, make_error("INVALID_JSON", "Unsupported request mode", {{"mode", mode}}));
    } catch (const std::exception& ex) {
        return error_response(request_id, make_error("INVALID_JSON", ex.what()));
    }
}

std::string JsonServiceAdapter::handle_json(const std::string& request_json) const {
    try {
        return handle(nlohmann::json::parse(request_json)).dump(2);
    } catch (const std::exception& ex) {
        return error_response("", make_error("INVALID_JSON", ex.what())).dump(2);
    }
}

nlohmann::json handle_request(const nlohmann::json& request) {
    return JsonServiceAdapter{}.handle(request);
}

std::string handle_request_json(const std::string& request_json) {
    return JsonServiceAdapter{}.handle_json(request_json);
}

} // namespace petri
