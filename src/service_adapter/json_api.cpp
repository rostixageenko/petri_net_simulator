#include "petri/service_adapter/json_api.hpp"

namespace petri {
namespace {

std::string request_id_from(const nlohmann::json& request) {
    if (request.is_object() && request.contains("request_id") && request.at("request_id").is_string()) {
        return request.at("request_id").get<std::string>();
    }
    return "";
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
    return PetriNet::from_json(request.at("net"));
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
    auto net_result = net_from_request(request);
    if (!net_result) {
        return error_response(request_id, net_result.error());
    }
    const PetriNet& net = net_result.value();

    SimulationParams params;
    const nlohmann::json params_json = request.contains("params") ? request.at("params") : nlohmann::json::object();
    if (params_json.contains("max_steps")) {
        params.max_steps = params_json.at("max_steps").get<std::size_t>();
    }
    if (params_json.contains("strategy")) {
        auto strategy = parse_strategy(params_json.at("strategy").get<std::string>());
        if (!strategy) {
            return error_response(request_id, strategy.error());
        }
        params.strategy = strategy.value();
    }
    if (params_json.contains("stop_on_deadlock")) {
        params.stop_on_deadlock = params_json.at("stop_on_deadlock").get<bool>();
    }
    if (params_json.contains("return_events")) {
        params.return_events = params_json.at("return_events").get<bool>();
    }
    if (params_json.contains("transition_id")) {
        params.transition_id = params_json.at("transition_id").get<std::string>();
    }

    Interpreter interpreter(net);
    auto run_result = interpreter.run(params);
    if (!run_result) {
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
    return result;
}

nlohmann::json algorithm_request(const nlohmann::json& request) {
    const std::string request_id = request_id_from(request);
    auto net_result = net_from_request(request);
    if (!net_result) {
        return error_response(request_id, net_result.error());
    }
    const PetriNet& net = net_result.value();

    const nlohmann::json params_json = request.contains("params") ? request.at("params") : nlohmann::json::object();
    const std::string graph_mode = params_json.value<std::string>("graph_mode", "reachability_graph");
    if (graph_mode != "reachability_graph") {
        return error_response(request_id, make_error("INVALID_JSON", "Only reachability_graph is supported in this slice",
                                                     {{"graph_mode", graph_mode}}));
    }

    ReachabilityOptions options;
    if (params_json.contains("max_states")) {
        options.max_states = params_json.at("max_states").get<std::size_t>();
    }
    if (params_json.contains("max_depth")) {
        options.max_depth = params_json.at("max_depth").get<std::size_t>();
    }

    auto reachability = build_reachability_graph(net, options);
    if (!reachability) {
        return error_response(request_id, reachability.error());
    }

    AlgorithmParams algorithm_params;
    const std::string source = params_json.value<std::string>("source", "initial");
    algorithm_params.source_vertex_id = source == "initial" ? reachability.value().initial_vertex_id : source;

    if (params_json.contains("target")) {
        algorithm_params.target_vertex_id = params_json.at("target").get<std::string>();
    } else if (params_json.contains("target_marking")) {
        auto target = find_marking_vertex(net, reachability.value(), params_json.at("target_marking"));
        if (!target) {
            return error_response(request_id, target.error());
        }
        algorithm_params.target_vertex_id = target.value();
    }

    const std::string algorithm = params_json.value<std::string>("algorithm", "bfs");
    auto algorithm_result = run_algorithm(algorithm, reachability.value().graph, algorithm_params);
    if (!algorithm_result) {
        return error_response(request_id, algorithm_result.error());
    }

    const auto& result_value = algorithm_result.value();
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
    return result;
}

nlohmann::json handle_request(const nlohmann::json& request) {
    const std::string request_id = request_id_from(request);
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
}

std::string handle_request_json(const std::string& request_json) {
    try {
        return handle_request(nlohmann::json::parse(request_json)).dump(2);
    } catch (const std::exception& ex) {
        return error_response("", make_error("INVALID_JSON", ex.what())).dump(2);
    }
}

} // namespace petri
