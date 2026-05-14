#include "petri/service_adapter/json_api.hpp"

#include <doctest/doctest.h>

#include <fstream>

using namespace petri;

namespace {

nlohmann::json load_mutex_json() {
    std::ifstream input("examples/mutex.json");
    nlohmann::json net;
    input >> net;
    return net;
}

nlohmann::json one_way_net_json() {
    return nlohmann::json::parse(R"({
        "name": "one way",
        "places": [
            {"id": "p_start", "tokens": 1},
            {"id": "p_done", "tokens": 0}
        ],
        "transitions": [
            {"id": "t_finish", "fire_time": 1.5}
        ],
        "arcs": [
            {"id": "a1", "source": "p_start", "target": "t_finish", "weight": 1},
            {"id": "a2", "source": "t_finish", "target": "p_done", "weight": 1}
        ]
    })");
}

} // namespace

TEST_CASE("simulate request returns visualization-friendly JSON") {
    nlohmann::json request = nlohmann::json::object();
    request["request_id"] = "run-001";
    request["mode"] = "simulate";
    request["net"] = load_mutex_json();
    request["params"]["max_steps"] = 2;
    request["params"]["strategy"] = "first_enabled";
    request["params"]["stop_on_deadlock"] = true;
    request["params"]["return_events"] = true;

    auto response = handle_request(request);
    CHECK_EQ(response.at("status").get<std::string>(), "ok");
    CHECK_EQ(response.at("events").size(), 2);
    CHECK_EQ(response.at("events").at(0).at("fired_transition").get<std::string>(), "t1_request");
    CHECK_EQ(response.at("final_marking").at("p1_cs").get<int>(), 1);
}

TEST_CASE("simulate request validates params and returns structured error") {
    nlohmann::json request = nlohmann::json::object();
    request["request_id"] = "bad-params";
    request["mode"] = "simulate";
    request["net"] = load_mutex_json();
    request["params"]["max_steps"] = -1;

    auto response = JsonServiceAdapter{}.handle(request);
    CHECK_EQ(response.at("status").get<std::string>(), "error");
    CHECK_EQ(response.at("error").at("code").get<std::string>(), "INVALID_JSON");
    CHECK_EQ(response.at("error").at("details").at("field").get<std::string>(), "max_steps");
}

TEST_CASE("algorithm request returns bfs path JSON") {
    nlohmann::json request = nlohmann::json::object();
    request["request_id"] = "alg-001";
    request["mode"] = "algorithm";
    request["net"] = load_mutex_json();
    request["params"]["graph_mode"] = "reachability_graph";
    request["params"]["algorithm"] = "bfs";
    request["params"]["max_states"] = 100;
    request["params"]["max_depth"] = 8;
    request["params"]["source"] = "initial";
    request["params"]["target_marking"]["p1_cs"] = 1;

    auto response = handle_request(request);
    CHECK_EQ(response.at("status").get<std::string>(), "ok");
    CHECK(response.at("found").get<bool>());
    CHECK_EQ(response.at("path").size(), 3);
    CHECK_EQ(response.at("path").at(1).at("via_transition").get<std::string>(), "t1_request");
    CHECK_EQ(response.at("metrics").at("path_length").get<int>(), 2);
}

TEST_CASE("algorithm request returns dijkstra metrics JSON") {
    nlohmann::json request = nlohmann::json::object();
    request["request_id"] = "alg-dijkstra";
    request["mode"] = "algorithm";
    request["net"] = load_mutex_json();
    request["params"]["graph_mode"] = "reachability_graph";
    request["params"]["algorithm"] = "dijkstra";
    request["params"]["max_states"] = 100;
    request["params"]["max_depth"] = 8;
    request["params"]["source"] = "initial";
    request["params"]["target_marking"]["p1_cs"] = 1;

    auto response = handle_request(request);
    CHECK_EQ(response.at("status").get<std::string>(), "ok");
    CHECK(response.at("found").get<bool>());
    CHECK(response.at("metrics").at("path_cost").get<double>() > 0.69);
    CHECK(response.at("metrics").at("path_cost").get<double>() < 0.71);
}

TEST_CASE("invalid request returns structured error") {
    nlohmann::json request = nlohmann::json::object();
    request["request_id"] = "bad-001";
    request["mode"] = "algorithm";
    request["net"] = load_mutex_json();
    request["params"]["algorithm"] = "unknown";

    auto response = handle_request(request);
    CHECK_EQ(response.at("status").get<std::string>(), "error");
    CHECK_EQ(response.at("error").at("code").get<std::string>(), "UNKNOWN_ALGORITHM");
}

TEST_CASE("invalid net is reported by service adapter") {
    nlohmann::json net = load_mutex_json();
    net.at("arcs").at(0)["weight"] = 0;

    nlohmann::json request = nlohmann::json::object();
    request["request_id"] = "bad-net";
    request["mode"] = "simulate";
    request["net"] = net;

    auto response = handle_request(request);
    CHECK_EQ(response.at("status").get<std::string>(), "error");
    CHECK_EQ(response.at("error").at("code").get<std::string>(), "INVALID_ARC_WEIGHT");
    CHECK_EQ(response.at("error").at("details").at("arc_id").get<std::string>(), "a1");
}

TEST_CASE("algorithm request reports unreachable target") {
    nlohmann::json request = nlohmann::json::object();
    request["request_id"] = "unreachable";
    request["mode"] = "algorithm";
    request["net"] = one_way_net_json();
    request["params"]["graph_mode"] = "reachability_graph";
    request["params"]["algorithm"] = "bfs";
    request["params"]["max_states"] = 10;
    request["params"]["max_depth"] = 3;
    request["params"]["source"] = "m1";
    request["params"]["target"] = "m0";

    auto response = handle_request(request);
    CHECK_EQ(response.at("status").get<std::string>(), "error");
    CHECK_EQ(response.at("error").at("code").get<std::string>(), "TARGET_NOT_FOUND");
    CHECK_EQ(response.at("error").at("details").at("source").get<std::string>(), "m1");
    CHECK_EQ(response.at("error").at("details").at("target").get<std::string>(), "m0");
}

TEST_CASE("algorithm request reports state limit exceeded") {
    nlohmann::json request = nlohmann::json::object();
    request["request_id"] = "state-limit";
    request["mode"] = "algorithm";
    request["net"] = one_way_net_json();
    request["params"]["graph_mode"] = "reachability_graph";
    request["params"]["algorithm"] = "bfs";
    request["params"]["max_states"] = 1;
    request["params"]["max_depth"] = 3;
    request["params"]["source"] = "initial";
    request["params"]["target_marking"]["p_done"] = 1;

    auto response = handle_request(request);
    CHECK_EQ(response.at("status").get<std::string>(), "error");
    CHECK_EQ(response.at("error").at("code").get<std::string>(), "STATE_LIMIT_EXCEEDED");
}

TEST_CASE("string entry point converts malformed JSON into error response") {
    auto response = nlohmann::json::parse(handle_request_json("{"));
    CHECK_EQ(response.at("status").get<std::string>(), "error");
    CHECK_EQ(response.at("error").at("code").get<std::string>(), "INVALID_JSON");
}
