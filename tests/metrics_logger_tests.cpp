#include "petri/logging_metrics/metrics_logger.hpp"
#include "petri/service_adapter/json_api.hpp"

#include <doctest/doctest.h>

#include <filesystem>

using namespace petri;

namespace {

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

nlohmann::json successful_algorithm_request() {
    nlohmann::json request = nlohmann::json::object();
    request["request_id"] = "metrics-success";
    request["mode"] = "algorithm";
    request["net"] = one_way_net_json();
    request["params"]["graph_mode"] = "reachability_graph";
    request["params"]["algorithm"] = "bfs";
    request["params"]["max_states"] = 10;
    request["params"]["max_depth"] = 3;
    request["params"]["source"] = "initial";
    request["params"]["target_marking"]["p_done"] = 1;
    return request;
}

} // namespace

TEST_CASE("creates metrics record") {
    RunMetricsInput input;
    input.request_id = "metrics-001";
    input.algorithm_name = "bfs";
    input.task_type = "reachability_graph";
    input.place_count = 2;
    input.transition_count = 1;
    input.arc_count = 2;
    input.built_state_count = 2;
    input.scanned_edge_count = 1;
    input.elapsed_ms = 0.25;
    input.found = true;
    input.path_cost = 1.5;
    input.path_length = 1;

    const auto record = create_metrics_record(input);
    CHECK_EQ(record.request_id, "metrics-001");
    CHECK_EQ(record.algorithm_name, "bfs");
    CHECK_EQ(record.task_type, "reachability_graph");
    CHECK_EQ(record.place_count, 2);
    CHECK_EQ(record.transition_count, 1);
    CHECK_EQ(record.arc_count, 2);
    CHECK_EQ(record.built_state_count, 2);
    CHECK_EQ(record.scanned_edge_count, 1);
    CHECK(record.found);
    CHECK_EQ(record.path_length, 1);

    const auto json = metrics_record_to_json(record);
    CHECK_EQ(json.at("algorithm").get<std::string>(), "bfs");
    CHECK_EQ(json.at("task_type").get<std::string>(), "reachability_graph");
    CHECK_EQ(json.at("places").get<int>(), 2);
    CHECK_EQ(json.at("path_cost").get<double>(), 1.5);
}

TEST_CASE("saves metrics record to JSONL file") {
    const std::string path = "logs/test_metrics_save.jsonl";
    std::filesystem::remove(path);

    auto record = create_metrics_record(RunMetricsInput{
        "metrics-save",
        "simulation",
        "simulation",
        2,
        1,
        2,
        2,
        0,
        0.5,
        true,
        1.5,
        1,
        "",
    });

    auto write = append_metrics_record(record, path);
    REQUIRE(write.has_value());

    auto read = read_metrics_records(path);
    REQUIRE(read.has_value());
    REQUIRE_EQ(read.value().size(), 1);
    CHECK_EQ(read.value().front().request_id, "metrics-save");
    CHECK_EQ(read.value().front().algorithm_name, "simulation");

    std::filesystem::remove(path);
}

TEST_CASE("service adapter writes metrics for successful algorithm run") {
    std::filesystem::remove(default_metrics_log_path());

    auto response = handle_request(successful_algorithm_request());
    REQUIRE_EQ(response.at("status").get<std::string>(), "ok");

    auto records = read_metrics_records();
    REQUIRE(records.has_value());
    REQUIRE_EQ(records.value().size(), 1);

    const auto& record = records.value().front();
    CHECK_EQ(record.request_id, "metrics-success");
    CHECK_EQ(record.algorithm_name, "bfs");
    CHECK_EQ(record.task_type, "reachability_graph");
    CHECK_EQ(record.place_count, 2);
    CHECK_EQ(record.transition_count, 1);
    CHECK_EQ(record.arc_count, 2);
    CHECK_EQ(record.built_state_count, 2);
    CHECK_EQ(record.scanned_edge_count, 1);
    CHECK(record.found);
    CHECK(record.error_message.empty());
    CHECK_EQ(record.path_length, 1);
    CHECK(record.path_cost > 1.49);
    CHECK(record.path_cost < 1.51);
}

TEST_CASE("service adapter writes metrics for failed algorithm run") {
    std::filesystem::remove(default_metrics_log_path());

    auto request = successful_algorithm_request();
    request["request_id"] = "metrics-error";
    request["params"]["algorithm"] = "astar";

    auto response = handle_request(request);
    REQUIRE_EQ(response.at("status").get<std::string>(), "error");
    CHECK_EQ(response.at("error").at("code").get<std::string>(), "UNKNOWN_ALGORITHM");

    auto records = read_metrics_records();
    REQUIRE(records.has_value());
    REQUIRE_EQ(records.value().size(), 1);

    const auto& record = records.value().front();
    CHECK_EQ(record.request_id, "metrics-error");
    CHECK_EQ(record.algorithm_name, "astar");
    CHECK_EQ(record.task_type, "reachability_graph");
    CHECK_EQ(record.place_count, 2);
    CHECK_EQ(record.transition_count, 1);
    CHECK_EQ(record.arc_count, 2);
    CHECK_EQ(record.built_state_count, 2);
    CHECK(!record.found);
    CHECK_EQ(record.path_length, 0);
    CHECK_EQ(record.path_cost, 0.0);
    CHECK_EQ(record.error_message, "Unknown algorithm");
}
