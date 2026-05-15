#include "petri/service_adapter/json_api.hpp"

#include <fstream>
#include <iostream>
#include <sstream>

namespace {

std::string choose_target_place(const nlohmann::json& net) {
    if (!net.is_object() || !net.contains("places") || !net.at("places").is_array()) {
        return "";
    }

    std::string fallback;
    for (const auto& place : net.at("places")) {
        if (!place.is_object() || !place.contains("id") || !place.at("id").is_string()) {
            continue;
        }
        const std::string id = place.at("id").get<std::string>();
        if (id == "p1_cs") {
            return id;
        }
        if (id == "p_done") {
            fallback = id;
        } else if (fallback.empty()) {
            fallback = id;
        }
    }
    return fallback;
}

} // namespace

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: petri_cli <net.json> [simulate|bfs|dfs|dijkstra]\n";
        return 2;
    }

    std::ifstream input(argv[1]);
    if (!input) {
        std::cerr << "Cannot open " << argv[1] << '\n';
        return 2;
    }

    nlohmann::json net;
    try {
        input >> net;
    } catch (const std::exception& ex) {
        std::cerr << "Invalid JSON: " << ex.what() << '\n';
        return 2;
    }

    const std::string mode = argc >= 3 ? argv[2] : "simulate";
    nlohmann::json request = nlohmann::json::object();
    request["request_id"] = "cli";
    request["net"] = net;

    if (mode == "simulate") {
        request["mode"] = "simulate";
        request["params"]["max_steps"] = 8;
        request["params"]["strategy"] = "round_robin";
        request["params"]["stop_on_deadlock"] = true;
        request["params"]["return_events"] = true;
    } else {
        request["mode"] = "algorithm";
        request["params"]["graph_mode"] = "reachability_graph";
        request["params"]["algorithm"] = mode;
        request["params"]["max_states"] = 100;
        request["params"]["max_depth"] = 10;
        request["params"]["source"] = "initial";
        const std::string target_place = choose_target_place(net);
        if (!target_place.empty()) {
            request["params"]["target_marking"][target_place] = 1;
        }
    }

    std::cout << petri::handle_request(request).dump(2) << '\n';
    return 0;
}
