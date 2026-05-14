#include "petri/runtime/algorithm_runtime.hpp"
#include "petri/logging_metrics/metrics_logger.hpp"

#include <doctest/doctest.h>

#include <algorithm>
#include <filesystem>
#include <utility>

using namespace petri;

namespace {

DirectedGraph small_graph() {
    DirectedGraph graph;
    graph.add_edge(GraphEdge{"e1", "a", "b", "", 1.0});
    graph.add_edge(GraphEdge{"e2", "b", "c", "", 2.0});
    graph.add_edge(GraphEdge{"e3", "a", "c", "", 5.0});
    return graph;
}

bool contains_name(const std::vector<std::string>& names, const std::string& name) {
    return std::find(names.begin(), names.end(), name) != names.end();
}

} // namespace

TEST_CASE("runtime registry exposes built-in algorithms") {
    const auto names = available_algorithms();

    CHECK(contains_name(names, "dfs"));
    CHECK(contains_name(names, "bfs"));
    CHECK(contains_name(names, "dijkstra"));
}

TEST_CASE("runtime runs built-in algorithm through unified task interface") {
    const auto graph = small_graph();

    AlgorithmTask task;
    task.algorithm_name = "bfs";
    task.graph = &graph;
    task.context.task_type = "unit_graph";

    auto result = run(task, AlgorithmParams{"a", std::string("c")});
    REQUIRE(result.has_value());
    CHECK_EQ(result.value().algorithm, "bfs");
    CHECK(result.value().found);
    CHECK_EQ(result.value().metrics.path_length, 1);
}

TEST_CASE("runtime registry can register a custom algorithm runner") {
    AlgorithmRegistry registry;
    auto registration = registry.register_algorithm(
        "custom",
        [](const DirectedGraph& graph, const AlgorithmParams& params) {
            AlgorithmResult result;
            result.algorithm = "custom";
            result.status = "ok";
            result.found = graph.has_vertex(params.source_vertex_id);
            if (result.found) {
                result.path.push_back(params.source_vertex_id);
            }
            return Result<AlgorithmResult>::success(std::move(result));
        });
    REQUIRE(registration.has_value());
    CHECK(registry.has_algorithm("custom"));
    CHECK(contains_name(registry.available_algorithms(), "custom"));

    const auto graph = small_graph();
    AlgorithmTask task;
    task.algorithm_name = "custom";
    task.graph = &graph;

    auto result = registry.run(task, AlgorithmParams{"a", std::nullopt});
    REQUIRE(result.has_value());
    CHECK(result.value().found);
    REQUIRE_EQ(result.value().path.size(), 1);
    CHECK_EQ(result.value().path.front(), "a");
}

TEST_CASE("runtime returns structured error for missing algorithm") {
    const auto graph = small_graph();

    AlgorithmTask task;
    task.algorithm_name = "astar";
    task.graph = &graph;

    auto result = run(task, AlgorithmParams{"a", std::string("c")});
    REQUIRE(!result.has_value());
    CHECK_EQ(result.error().code, "UNKNOWN_ALGORITHM");
    CHECK_EQ(result.error().details.at("algorithm"), "astar");
}

TEST_CASE("runtime writes metrics when task context enables logging") {
    const std::string path = "logs/test_runtime_metrics.jsonl";
    std::filesystem::remove(path);

    const auto graph = small_graph();
    AlgorithmTask task;
    task.algorithm_name = "dijkstra";
    task.graph = &graph;
    task.context.request_id = "runtime-metrics";
    task.context.task_type = "unit_graph";
    task.context.place_count = 3;
    task.context.transition_count = 0;
    task.context.arc_count = 3;
    task.context.built_state_count = graph.vertex_count();
    task.context.log_metrics = true;
    task.context.metrics_log_path = path;

    auto result = run(task, AlgorithmParams{"a", std::string("c")});
    REQUIRE(result.has_value());
    REQUIRE(result.value().found);

    auto records = read_metrics_records(path);
    REQUIRE(records.has_value());
    REQUIRE_EQ(records.value().size(), 1);

    const auto& record = records.value().front();
    CHECK_EQ(record.request_id, "runtime-metrics");
    CHECK_EQ(record.algorithm_name, "dijkstra");
    CHECK_EQ(record.task_type, "unit_graph");
    CHECK_EQ(record.place_count, 3);
    CHECK_EQ(record.arc_count, 3);
    CHECK_EQ(record.built_state_count, graph.vertex_count());
    CHECK(record.scanned_edge_count > 0);
    CHECK(record.found);
    CHECK(record.error_message.empty());
    CHECK_EQ(record.path_length, result.value().metrics.path_length);
    CHECK_EQ(record.path_cost, result.value().metrics.path_cost);

    std::filesystem::remove(path);
}
