#include "petri/algorithm_selector/algorithm_selector.hpp"
#include "petri/core_pn/interpreter.hpp"
#include "petri/core_pn/petri_net.hpp"
#include "petri/graph_models/reachability.hpp"
#include "petri/service_adapter/json_api.hpp"

#include <pybind11/pybind11.h>

#include <string>
#include <utility>
#include <vector>

namespace py = pybind11;

namespace {

nlohmann::json py_to_json(const py::handle& object) {
    if (object.is_none()) {
        return nlohmann::json();
    }
    if (py::isinstance<py::str>(object)) {
        return nlohmann::json::parse(object.cast<std::string>());
    }

    const auto json_module = py::module_::import("json");
    const std::string dumped = json_module.attr("dumps")(object).cast<std::string>();
    return nlohmann::json::parse(dumped);
}

py::object json_to_py(const nlohmann::json& data) {
    const auto json_module = py::module_::import("json");
    return json_module.attr("loads")(data.dump());
}

nlohmann::json params_or_empty(const py::handle& params) {
    if (params.is_none()) {
        return nlohmann::json::object();
    }

    auto result = py_to_json(params);
    if (!result.is_object()) {
        throw py::value_error("params must be a JSON object");
    }
    return result;
}

[[noreturn]] void throw_result_error(const petri::Error& error) {
    throw py::value_error(error.code + ": " + error.message);
}

petri::PetriNet net_from_python(const py::handle& net_object) {
    auto net_json = py_to_json(net_object);
    auto net = petri::PetriNet::from_json(net_json);
    if (!net) {
        throw_result_error(net.error());
    }
    return net.value();
}

petri::Marking marking_from_json(const petri::PetriNet& net, const nlohmann::json& data) {
    if (data.is_null()) {
        return net.initial_marking();
    }
    if (!data.is_object()) {
        throw py::value_error("marking must be a JSON object keyed by place id");
    }

    std::vector<int> tokens(net.places().size(), 0);
    for (std::size_t i = 0; i < net.places().size(); ++i) {
        const auto& place = net.places()[i];
        if (!data.contains(place.id)) {
            continue;
        }
        if (!data.at(place.id).is_number_integer()) {
            throw py::value_error("marking token value must be an integer for place '" + place.id + "'");
        }
        const int value = data.at(place.id).get<int>();
        if (value < 0) {
            throw py::value_error("marking token value must be non-negative for place '" + place.id + "'");
        }
        tokens[i] = value;
    }

    return petri::Marking(std::move(tokens));
}

petri::Marking marking_from_python(const petri::PetriNet& net, const py::handle& marking) {
    if (marking.is_none()) {
        return net.initial_marking();
    }
    return marking_from_json(net, py_to_json(marking));
}

std::size_t read_size_param(const nlohmann::json& params, const std::string& field,
                            std::size_t default_value) {
    if (!params.contains(field)) {
        return default_value;
    }
    if (!params.at(field).is_number_integer()) {
        throw py::value_error("params." + field + " must be a non-negative integer");
    }
    const int value = params.at(field).get<int>();
    if (value < 0) {
        throw py::value_error("params." + field + " must be a non-negative integer");
    }
    return static_cast<std::size_t>(value);
}

bool read_bool_param(const nlohmann::json& params, const std::string& field, bool default_value) {
    if (!params.contains(field)) {
        return default_value;
    }
    if (!params.at(field).is_boolean()) {
        throw py::value_error("params." + field + " must be a boolean");
    }
    return params.at(field).get<bool>();
}

double read_double_param(const nlohmann::json& params, const std::string& field, double default_value) {
    if (!params.contains(field)) {
        return default_value;
    }
    if (!params.at(field).is_number()) {
        throw py::value_error("params." + field + " must be a number");
    }
    return params.at(field).get<double>();
}

std::string read_string_param(const nlohmann::json& params, const std::string& field,
                              const std::string& default_value) {
    if (!params.contains(field)) {
        return default_value;
    }
    if (!params.at(field).is_string()) {
        throw py::value_error("params." + field + " must be a string");
    }
    return params.at(field).get<std::string>();
}

std::vector<std::string> read_algorithms(const nlohmann::json& params) {
    std::vector<std::string> algorithms;
    if (!params.contains("algorithms")) {
        return algorithms;
    }
    if (!params.at("algorithms").is_array()) {
        throw py::value_error("params.algorithms must be an array of strings");
    }
    for (const auto& item : params.at("algorithms")) {
        if (!item.is_string()) {
            throw py::value_error("params.algorithms must be an array of strings");
        }
        algorithms.push_back(item.get<std::string>());
    }
    return algorithms;
}

petri::SelectionWeights read_selection_weights(const nlohmann::json& params) {
    const nlohmann::json weights_json =
        params.contains("weights") && params.at("weights").is_object() ? params.at("weights") : params;

    petri::SelectionWeights weights;
    weights.elapsed_ms = read_double_param(weights_json, "elapsed_ms", weights.elapsed_ms);
    weights.path_length = read_double_param(weights_json, "path_length", weights.path_length);
    weights.path_cost = read_double_param(weights_json, "path_cost", weights.path_cost);
    weights.visited_vertices = read_double_param(weights_json, "visited_vertices", weights.visited_vertices);
    return weights;
}

py::object load_petri_net_from_json_py(const py::object& net_object) {
    const auto net = net_from_python(net_object);

    nlohmann::json result = nlohmann::json::object();
    result["status"] = "ok";
    result["model_name"] = net.name();
    result["initial_marking"] = net.marking_to_json(net.initial_marking());
    result["enabled_transitions"] = nlohmann::json::array();
    for (const auto& transition_id : net.enabled_transitions(net.initial_marking())) {
        result["enabled_transitions"].push_back(transition_id);
    }
    return json_to_py(result);
}

py::object get_enabled_transitions_py(const py::object& net_object, const py::object& marking) {
    const auto net = net_from_python(net_object);
    const auto state = marking_from_python(net, marking);

    nlohmann::json result = nlohmann::json::array();
    for (const auto& transition_id : net.enabled_transitions(state)) {
        result.push_back(transition_id);
    }
    return json_to_py(result);
}

py::object fire_transition_py(const py::object& net_object, const std::string& transition_id,
                              const py::object& marking) {
    const auto net = net_from_python(net_object);
    const auto state = marking_from_python(net, marking);
    auto next = net.fire_transition(state, transition_id);
    if (!next) {
        throw_result_error(next.error());
    }
    return json_to_py(net.marking_to_json(next.value()));
}

py::object run_simulation_py(const py::object& net_object, const py::object& params) {
    nlohmann::json request = nlohmann::json::object();
    request["mode"] = "simulate";
    request["net"] = py_to_json(net_object);
    request["params"] = params_or_empty(params);
    return json_to_py(petri::simulate_request(request));
}

py::object run_algorithm_py(const py::object& net_object, const py::object& params) {
    nlohmann::json request = nlohmann::json::object();
    request["mode"] = "algorithm";
    request["net"] = py_to_json(net_object);
    request["params"] = params_or_empty(params);
    return json_to_py(petri::algorithm_request(request));
}

py::object select_algorithm_py(const py::object& net_object, const py::object& params_object) {
    const auto net = net_from_python(net_object);
    const auto params = params_or_empty(params_object);

    const std::string graph_mode = read_string_param(params, "graph_mode", "reachability_graph");
    if (graph_mode != "reachability_graph") {
        throw py::value_error("Only reachability_graph is supported");
    }

    petri::ReachabilityOptions options;
    options.max_states = read_size_param(params, "max_states", options.max_states);
    options.max_depth = read_size_param(params, "max_depth", options.max_depth);

    auto reachability = petri::build_reachability_graph(net, options);
    if (!reachability) {
        throw_result_error(reachability.error());
    }

    petri::AlgorithmParams algorithm_params;
    const std::string source = read_string_param(params, "source", "initial");
    algorithm_params.source_vertex_id =
        source == "initial" ? reachability.value().initial_vertex_id : source;
    if (params.contains("target")) {
        if (!params.at("target").is_string()) {
            throw py::value_error("params.target must be a string");
        }
        algorithm_params.target_vertex_id = params.at("target").get<std::string>();
    } else if (params.contains("target_marking")) {
        auto target = petri::find_marking_vertex(net, reachability.value(), params.at("target_marking"));
        if (!target) {
            throw_result_error(target.error());
        }
        algorithm_params.target_vertex_id = target.value();
    }

    petri::AlgorithmSelectionRequest request;
    request.task.graph = &reachability.value().graph;
    request.task.context.task_type = graph_mode;
    request.task.context.place_count = net.places().size();
    request.task.context.transition_count = net.transitions().size();
    request.task.context.arc_count = net.arcs().size();
    request.task.context.built_state_count = reachability.value().graph.vertex_count();
    request.task.context.log_metrics = read_bool_param(params, "log_metrics", false);
    request.params = algorithm_params;
    request.algorithms = read_algorithms(params);
    request.weights = read_selection_weights(params);
    request.require_found = read_bool_param(params, "require_found", true);

    auto selection = petri::select_algorithm(request);
    if (!selection) {
        throw_result_error(selection.error());
    }
    return json_to_py(petri::algorithm_selection_report_to_json(selection.value()));
}

} // namespace

PYBIND11_MODULE(petri_core, module) {
    module.doc() = "Python bindings for the petri_net_simulator C++ core";

    module.def("load_petri_net_from_json", &load_petri_net_from_json_py, py::arg("net"));
    module.def("get_enabled_transitions", &get_enabled_transitions_py, py::arg("net"),
               py::arg("marking") = py::none());
    module.def("fire_transition", &fire_transition_py, py::arg("net"), py::arg("transition_id"),
               py::arg("marking") = py::none());
    module.def("run_simulation", &run_simulation_py, py::arg("net"), py::arg("params") = py::none());
    module.def("run_algorithm", &run_algorithm_py, py::arg("net"), py::arg("params") = py::none());
    module.def("select_algorithm", &select_algorithm_py, py::arg("net"), py::arg("params") = py::none());
}
