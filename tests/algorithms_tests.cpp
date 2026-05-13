#include "petri/algorithms/algorithms.hpp"
#include "petri/runtime/algorithm_runtime.hpp"

#include <doctest/doctest.h>

using namespace petri;

TEST_CASE("bfs finds shortest unweighted path") {
    DirectedGraph graph;
    graph.add_edge(GraphEdge{"e1", "a", "b", "", 5.0});
    graph.add_edge(GraphEdge{"e2", "a", "c", "", 1.0});
    graph.add_edge(GraphEdge{"e3", "c", "d", "", 1.0});
    graph.add_edge(GraphEdge{"e4", "b", "d", "", 1.0});

    auto result = run_bfs(graph, AlgorithmParams{"a", std::string("d")});
    REQUIRE(result.has_value());
    CHECK(result.value().found);
    CHECK_EQ(result.value().metrics.path_length, 2);
    CHECK_EQ(result.value().path.front(), "a");
    CHECK_EQ(result.value().path.back(), "d");
}

TEST_CASE("dfs traverses reachable target") {
    DirectedGraph graph;
    graph.add_edge(GraphEdge{"e1", "a", "b", "", 1.0});
    graph.add_edge(GraphEdge{"e2", "b", "c", "", 1.0});

    auto result = run_dfs(graph, AlgorithmParams{"a", std::string("c")});
    REQUIRE(result.has_value());
    CHECK(result.value().found);
    CHECK_EQ(result.value().metrics.path_length, 2);
}

TEST_CASE("dijkstra uses edge weights") {
    DirectedGraph graph;
    graph.add_edge(GraphEdge{"e1", "a", "b", "", 10.0});
    graph.add_edge(GraphEdge{"e2", "a", "c", "", 1.0});
    graph.add_edge(GraphEdge{"e3", "c", "b", "", 2.0});

    auto result = run_algorithm("dijkstra", graph, AlgorithmParams{"a", std::string("b")});
    REQUIRE(result.has_value());
    CHECK(result.value().found);
    CHECK_EQ(result.value().metrics.path_length, 2);
    CHECK(result.value().metrics.path_cost > 2.99);
    CHECK(result.value().metrics.path_cost < 3.01);
}

TEST_CASE("runtime rejects unknown algorithm") {
    DirectedGraph graph;
    graph.add_vertex("a");
    auto result = run_algorithm("astar", graph, AlgorithmParams{"a", std::nullopt});
    REQUIRE(!result.has_value());
    CHECK_EQ(result.error().code, "UNKNOWN_ALGORITHM");
}
