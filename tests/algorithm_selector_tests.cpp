#include "petri/algorithm_selector/algorithm_selector.hpp"

#include <doctest/doctest.h>

#include <algorithm>
#include <string>

using namespace petri;

namespace {

DirectedGraph weighted_choice_graph() {
    DirectedGraph graph;
    graph.add_edge(GraphEdge{"e1", "a", "c", "", 1.0});
    graph.add_edge(GraphEdge{"e2", "a", "b", "", 100.0});
    graph.add_edge(GraphEdge{"e3", "c", "b", "", 1.0});
    return graph;
}

AlgorithmSelectionRequest make_request(const DirectedGraph& graph, SelectionWeights weights) {
    AlgorithmSelectionRequest request;
    request.task.graph = &graph;
    request.task.context.task_type = "unit_graph";
    request.params.source_vertex_id = "a";
    request.params.target_vertex_id = std::string("b");
    request.algorithms = {"bfs", "dijkstra", "dfs"};
    request.weights = weights;
    return request;
}

const AlgorithmCandidateReport* find_candidate(const AlgorithmSelectionReport& report,
                                               const std::string& algorithm) {
    const auto it = std::find_if(report.candidates.begin(), report.candidates.end(),
                                 [&algorithm](const AlgorithmCandidateReport& candidate) {
                                     return candidate.algorithm == algorithm;
                                 });
    return it == report.candidates.end() ? nullptr : &(*it);
}

} // namespace

TEST_CASE("algorithm selector prefers BFS when path length has priority") {
    const auto graph = weighted_choice_graph();
    auto request = make_request(graph, SelectionWeights{0.0, 1.0, 0.0, 0.0});

    auto selection = select_algorithm(request);
    REQUIRE(selection.has_value());

    const auto& report = selection.value();
    REQUIRE(report.best_algorithm.has_value());
    CHECK_EQ(*report.best_algorithm, "bfs");
    REQUIRE(report.best_result.has_value());
    CHECK_EQ(report.best_result->metrics.path_length, 1);

    const auto* bfs = find_candidate(report, "bfs");
    const auto* dijkstra = find_candidate(report, "dijkstra");
    REQUIRE(bfs != nullptr);
    REQUIRE(dijkstra != nullptr);
    CHECK(bfs->score < dijkstra->score);
}

TEST_CASE("algorithm selector prefers Dijkstra when path cost has priority") {
    const auto graph = weighted_choice_graph();
    auto request = make_request(graph, SelectionWeights{0.0, 0.0, 1.0, 0.0});

    auto selection = select_algorithm(request);
    REQUIRE(selection.has_value());

    const auto& report = selection.value();
    REQUIRE(report.best_algorithm.has_value());
    CHECK_EQ(*report.best_algorithm, "dijkstra");
    REQUIRE(report.best_result.has_value());
    CHECK(report.best_result->metrics.path_cost > 1.99);
    CHECK(report.best_result->metrics.path_cost < 2.01);

    const auto* bfs = find_candidate(report, "bfs");
    const auto* dijkstra = find_candidate(report, "dijkstra");
    REQUIRE(bfs != nullptr);
    REQUIRE(dijkstra != nullptr);
    CHECK(dijkstra->score < bfs->score);
}

TEST_CASE("algorithm selector returns JSON comparison report") {
    const auto graph = weighted_choice_graph();
    auto request = make_request(graph, SelectionWeights{0.0, 0.0, 1.0, 0.0});

    auto selection = select_algorithm(request);
    REQUIRE(selection.has_value());

    const auto report_json = algorithm_selection_report_to_json(selection.value());

    CHECK_EQ(report_json.at("best_algorithm").get<std::string>(), "dijkstra");
    REQUIRE(report_json.at("comparison").is_array());
    CHECK_EQ(report_json.at("comparison").size(), 3);
    CHECK(report_json.at("criteria").contains("path_cost"));
    CHECK(report_json.at("best_result").at("metrics").contains("path_cost"));
    CHECK(report_json.at("comparison").at(0).contains("score"));
    CHECK(report_json.at("comparison").at(0).contains("metrics"));
}

TEST_CASE("algorithm selector records failed candidates in comparison") {
    const auto graph = weighted_choice_graph();
    AlgorithmSelectionRequest request;
    request.task.graph = &graph;
    request.params.source_vertex_id = "a";
    request.params.target_vertex_id = std::string("b");
    request.algorithms = {"astar", "dijkstra"};
    request.weights.path_cost = 1.0;

    auto selection = select_algorithm(request);
    REQUIRE(selection.has_value());

    const auto& report = selection.value();
    REQUIRE(report.best_algorithm.has_value());
    CHECK_EQ(*report.best_algorithm, "dijkstra");

    const auto* astar = find_candidate(report, "astar");
    REQUIRE(astar != nullptr);
    CHECK_EQ(astar->status, "error");
    CHECK(!astar->eligible);
    CHECK(!astar->error_message.empty());
}
