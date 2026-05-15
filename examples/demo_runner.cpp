#include "petri/core_pn/interpreter.hpp"
#include "petri/core_pn/petri_net.hpp"
#include "petri/graph_models/reachability.hpp"
#include "petri/runtime/algorithm_runtime.hpp"
#include "petri/service_adapter/json_api.hpp"

#include <fstream>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void print_section(const std::string& title) {
    std::cout << "\n=== " << title << " ===\n";
}

void print_error(const petri::Error& error) {
    std::cerr << "Error [" << error.code << "]: " << error.message << '\n';
    for (const auto& [key, value] : error.details) {
        std::cerr << "  " << key << ": " << value << '\n';
    }
}

void print_ids(const std::vector<std::string>& ids) {
    if (ids.empty()) {
        std::cout << "(none)";
        return;
    }
    for (std::size_t i = 0; i < ids.size(); ++i) {
        if (i != 0) {
            std::cout << " -> ";
        }
        std::cout << ids[i];
    }
}

std::optional<std::string> choose_target_place(const petri::PetriNet& net) {
    if (net.has_place("p1_cs")) {
        return "p1_cs";
    }
    if (net.has_place("p_done")) {
        return "p_done";
    }
    if (!net.places().empty()) {
        return net.places().back().id;
    }
    return std::nullopt;
}

nlohmann::json load_json_file(const std::string& path) {
    std::ifstream input(path);
    if (!input) {
        throw std::runtime_error("cannot open input JSON file: " + path);
    }

    nlohmann::json data;
    input >> data;
    return data;
}

nlohmann::json make_algorithm_request(const nlohmann::json& net_json,
                                      const std::string& algorithm,
                                      const std::optional<std::string>& target_place) {
    nlohmann::json request = nlohmann::json::object();
    request["request_id"] = "demo-" + algorithm;
    request["mode"] = "algorithm";
    request["net"] = net_json;
    request["params"]["graph_mode"] = "reachability_graph";
    request["params"]["algorithm"] = algorithm;
    request["params"]["max_states"] = 100;
    request["params"]["max_depth"] = 8;
    request["params"]["source"] = "initial";
    if (target_place) {
        request["params"]["target_marking"][*target_place] = 1;
    }
    return request;
}

} // namespace

int main(int argc, char** argv) {
    const std::string input_path = argc >= 2 ? argv[1] : "examples/mutex.json";

    nlohmann::json net_json;
    try {
        net_json = load_json_file(input_path);
    } catch (const std::exception& ex) {
        std::cerr << ex.what() << '\n';
        return 1;
    }

    auto net_result = petri::PetriNet::from_json(net_json);
    if (!net_result) {
        print_error(net_result.error());
        return 1;
    }
    const auto& net = net_result.value();

    print_section("Input model");
    std::cout << "File: " << input_path << '\n';
    std::cout << "Model name: " << net.name() << '\n';
    std::cout << "Places: " << net.places().size()
              << ", transitions: " << net.transitions().size()
              << ", arcs: " << net.arcs().size() << '\n';

    const auto initial_marking = net.initial_marking();
    print_section("Initial marking");
    std::cout << net.marking_to_json(initial_marking).dump(2) << '\n';

    print_section("Enabled transitions at start");
    const auto enabled = net.enabled_transitions(initial_marking);
    print_ids(enabled);
    std::cout << '\n';

    print_section("Simulation: round_robin, max 4 steps");
    petri::Interpreter interpreter(net);
    petri::SimulationParams simulation_params;
    simulation_params.max_steps = 4;
    simulation_params.strategy = petri::SimulationStrategy::RoundRobin;
    simulation_params.stop_on_deadlock = true;
    simulation_params.return_events = true;

    auto simulation = interpreter.run(simulation_params);
    if (!simulation) {
        print_error(simulation.error());
        return 1;
    }

    for (const auto& event : simulation.value().events) {
        std::cout << "Step " << event.step
                  << ": fired " << event.fired_transition
                  << ", time=" << event.time << '\n';
        std::cout << "  before: " << net.marking_to_json(event.marking_before).dump() << '\n';
        std::cout << "  after:  " << net.marking_to_json(event.marking_after).dump() << '\n';
    }
    std::cout << "Steps executed: " << simulation.value().steps_executed
              << ", deadlock: " << (simulation.value().deadlock ? "true" : "false") << '\n';

    print_section("Reachability graph");
    auto reachability = petri::build_reachability_graph(net, petri::ReachabilityOptions{100, 8});
    if (!reachability) {
        print_error(reachability.error());
        return 1;
    }
    std::cout << "Initial vertex: " << reachability.value().initial_vertex_id << '\n';
    std::cout << "Vertices: " << reachability.value().graph.vertex_count()
              << ", edges: " << reachability.value().graph.edge_count() << '\n';

    const auto target_place = choose_target_place(net);
    petri::AlgorithmParams algorithm_params;
    algorithm_params.source_vertex_id = reachability.value().initial_vertex_id;
    if (target_place) {
        nlohmann::json target_marking = nlohmann::json::object();
        target_marking[*target_place] = 1;
        auto target_vertex = petri::find_marking_vertex(net, reachability.value(), target_marking);
        if (target_vertex) {
            algorithm_params.target_vertex_id = target_vertex.value();
            std::cout << "Target marking: " << target_marking.dump()
                      << " at vertex " << target_vertex.value() << '\n';
        } else {
            std::cout << "Target marking " << target_marking.dump()
                      << " was not found; algorithms will print traversal order.\n";
        }
    }

    print_section("Algorithms");
    for (const std::string& algorithm : {"dfs", "bfs", "dijkstra"}) {
        auto result = petri::run_algorithm(algorithm, reachability.value().graph, algorithm_params);
        if (!result) {
            std::cout << algorithm << ": ";
            print_error(result.error());
            continue;
        }

        const auto& value = result.value();
        std::cout << algorithm << ": status=" << value.status
                  << ", found=" << (value.found ? "true" : "false")
                  << ", path_length=" << value.metrics.path_length
                  << ", path_cost=" << value.metrics.path_cost << '\n';
        std::cout << "  path: ";
        print_ids(value.path);
        std::cout << '\n';
    }

    print_section("JSON response from service_adapter (BFS)");
    const auto response = petri::handle_request(make_algorithm_request(net_json, "bfs", target_place));
    std::cout << response.dump(2) << '\n';

    print_section("Where to look next");
    std::cout << "Metrics log, if written by service_adapter: logs/metrics.jsonl\n";
    std::cout << "Try simple model too: .\\build\\Debug\\demo_runner.exe examples\\simple_chain.json\n";
    return 0;
}
