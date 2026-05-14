#include "petri/service_adapter/json_api.hpp"

#include <doctest/doctest.h>

using namespace petri;

namespace {

nlohmann::json contract_net() {
    return nlohmann::json::parse(R"({
        "name": "demo_net",
        "places": [
            {"id": "p_start", "label": "Start", "tokens": 1},
            {"id": "p_done", "label": "Done", "tokens": 0}
        ],
        "transitions": [
            {"id": "t_finish", "label": "Finish", "fire_time": 0.5}
        ],
        "arcs": [
            {"id": "a1", "source": "p_start", "target": "t_finish", "weight": 1},
            {"id": "a2", "source": "t_finish", "target": "p_done", "weight": 1}
        ]
    })");
}

nlohmann::json simulation_request() {
    nlohmann::json request = nlohmann::json::object();
    request["request_id"] = "sim-contract";
    request["mode"] = "simulate";
    request["net"] = contract_net();
    request["params"]["max_steps"] = 1;
    request["params"]["strategy"] = "first_enabled";
    request["params"]["stop_on_deadlock"] = true;
    request["params"]["return_events"] = true;
    return request;
}

nlohmann::json algorithm_request_for(const std::string& request_id, const std::string& algorithm) {
    nlohmann::json request = nlohmann::json::object();
    request["request_id"] = request_id;
    request["mode"] = "algorithm";
    request["net"] = contract_net();
    request["params"]["graph_mode"] = "reachability_graph";
    request["params"]["algorithm"] = algorithm;
    request["params"]["max_states"] = 20;
    request["params"]["max_depth"] = 4;
    request["params"]["source"] = "initial";
    request["params"]["target_marking"]["p_done"] = 1;
    return request;
}

void check_route_response_contract(const nlohmann::json& response, const std::string& algorithm) {
    CHECK_EQ(response.at("status").get<std::string>(), "ok");
    CHECK_EQ(response.at("algorithm").get<std::string>(), algorithm);
    CHECK_EQ(response.at("graph_mode").get<std::string>(), "reachability_graph");
    CHECK(response.at("found").get<bool>());

    REQUIRE(response.at("path").is_array());
    REQUIRE_EQ(response.at("path").size(), 2);
    CHECK_EQ(response.at("path").at(0).at("vertex_id").get<std::string>(), "m0");
    CHECK_EQ(response.at("path").at(0).at("marking").at("p_start").get<int>(), 1);
    CHECK_EQ(response.at("path").at(0).at("marking").at("p_done").get<int>(), 0);
    CHECK_EQ(response.at("path").at(1).at("via_transition").get<std::string>(), "t_finish");
    CHECK_EQ(response.at("path").at(1).at("marking").at("p_start").get<int>(), 0);
    CHECK_EQ(response.at("path").at(1).at("marking").at("p_done").get<int>(), 1);

    CHECK(response.at("metrics").contains("elapsed_ms"));
    CHECK(response.at("metrics").at("visited_vertices").get<int>() >= 1);
    CHECK(response.at("metrics").at("visited_vertices").get<int>() <= 2);
    CHECK(response.at("metrics").at("scanned_edges").get<int>() >= 1);
    CHECK_EQ(response.at("metrics").at("path_length").get<int>(), 1);
    CHECK(response.at("metrics").at("path_cost").get<double>() > 0.49);
    CHECK(response.at("metrics").at("path_cost").get<double>() < 0.51);

    CHECK_EQ(response.at("graph").at("vertices").get<int>(), 2);
    CHECK_EQ(response.at("graph").at("edges").get<int>(), 1);
}

} // namespace

TEST_CASE("integration contract simulation request returns documented response shape") {
    const auto response = handle_request(simulation_request());

    CHECK_EQ(response.at("request_id").get<std::string>(), "sim-contract");
    CHECK_EQ(response.at("status").get<std::string>(), "ok");
    CHECK_EQ(response.at("model_name").get<std::string>(), "demo_net");
    CHECK_EQ(response.at("initial_marking").at("p_start").get<int>(), 1);
    CHECK_EQ(response.at("initial_marking").at("p_done").get<int>(), 0);
    CHECK_EQ(response.at("final_marking").at("p_start").get<int>(), 0);
    CHECK_EQ(response.at("final_marking").at("p_done").get<int>(), 1);

    REQUIRE(response.at("events").is_array());
    REQUIRE_EQ(response.at("events").size(), 1);
    CHECK_EQ(response.at("events").at(0).at("step").get<int>(), 1);
    CHECK_EQ(response.at("events").at(0).at("fired_transition").get<std::string>(), "t_finish");
    CHECK_EQ(response.at("events").at(0).at("marking_before").at("p_start").get<int>(), 1);
    CHECK_EQ(response.at("events").at(0).at("marking_after").at("p_done").get<int>(), 1);
    CHECK_EQ(response.at("metrics").at("steps_executed").get<int>(), 1);
    CHECK(response.at("metrics").contains("elapsed_ms"));
    CHECK(response.at("metrics").contains("deadlock"));
}

TEST_CASE("integration contract BFS request returns route metrics and states") {
    const auto response = handle_request(algorithm_request_for("bfs-contract", "bfs"));

    CHECK_EQ(response.at("request_id").get<std::string>(), "bfs-contract");
    check_route_response_contract(response, "bfs");
}

TEST_CASE("integration contract Dijkstra request returns route metrics and states") {
    const auto response = handle_request(algorithm_request_for("dijkstra-contract", "dijkstra"));

    CHECK_EQ(response.at("request_id").get<std::string>(), "dijkstra-contract");
    check_route_response_contract(response, "dijkstra");
}

TEST_CASE("integration contract string adapter accepts JSON request") {
    const std::string request_json = simulation_request().dump();
    const auto response = nlohmann::json::parse(handle_request_json(request_json));

    CHECK_EQ(response.at("status").get<std::string>(), "ok");
    CHECK_EQ(response.at("request_id").get<std::string>(), "sim-contract");
    CHECK_EQ(response.at("final_marking").at("p_done").get<int>(), 1);
}

TEST_CASE("integration contract returns structured error response") {
    auto request = algorithm_request_for("bad-algorithm-contract", "astar");
    const auto response = handle_request(request);

    CHECK_EQ(response.at("request_id").get<std::string>(), "bad-algorithm-contract");
    CHECK_EQ(response.at("status").get<std::string>(), "error");
    CHECK_EQ(response.at("error").at("code").get<std::string>(), "UNKNOWN_ALGORITHM");
    CHECK(response.at("error").contains("message"));
    CHECK(response.at("error").contains("details"));
}
