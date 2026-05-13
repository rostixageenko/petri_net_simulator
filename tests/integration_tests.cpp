#include "petri/graph_models/reachability.hpp"
#include "petri/runtime/algorithm_runtime.hpp"

#include <doctest/doctest.h>

using namespace petri;

TEST_CASE("builds reachability graph for mutex example") {
    auto net = PetriNet::load_from_file("examples/mutex.json");
    REQUIRE(net.has_value());

    auto reachability = build_reachability_graph(net.value(), ReachabilityOptions{100, 8});
    REQUIRE(reachability.has_value());
    CHECK(reachability.value().graph.vertex_count() > 1);
    CHECK(reachability.value().graph.edge_count() > 1);
    CHECK_EQ(reachability.value().initial_vertex_id, "m0");

    auto target = find_marking_vertex(
        net.value(), reachability.value(), nlohmann::json::parse(R"({"p1_cs": 1})"));
    REQUIRE(target.has_value());
}

TEST_CASE("bfs reaches critical section marking") {
    auto net = PetriNet::load_from_file("examples/mutex.json");
    REQUIRE(net.has_value());
    auto reachability = build_reachability_graph(net.value(), ReachabilityOptions{100, 8});
    REQUIRE(reachability.has_value());
    auto target = find_marking_vertex(
        net.value(), reachability.value(), nlohmann::json::parse(R"({"p1_cs": 1})"));
    REQUIRE(target.has_value());

    auto result = run_algorithm(
        "bfs", reachability.value().graph,
        AlgorithmParams{reachability.value().initial_vertex_id, target.value()});
    REQUIRE(result.has_value());
    CHECK(result.value().found);
    CHECK_EQ(result.value().metrics.path_length, 2);
}

TEST_CASE("dijkstra uses transition fire_time in reachability graph") {
    auto net = PetriNet::load_from_file("examples/mutex.json");
    REQUIRE(net.has_value());
    auto reachability = build_reachability_graph(net.value(), ReachabilityOptions{100, 8});
    REQUIRE(reachability.has_value());
    auto target = find_marking_vertex(
        net.value(), reachability.value(), nlohmann::json::parse(R"({"p1_cs": 1})"));
    REQUIRE(target.has_value());

    auto result = run_algorithm(
        "dijkstra", reachability.value().graph,
        AlgorithmParams{reachability.value().initial_vertex_id, target.value()});
    REQUIRE(result.has_value());
    CHECK(result.value().found);
    CHECK_EQ(result.value().metrics.path_length, 2);
    CHECK(result.value().metrics.path_cost > 0.69);
    CHECK(result.value().metrics.path_cost < 0.71);
}
