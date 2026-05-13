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
