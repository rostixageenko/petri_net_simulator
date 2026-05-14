#include "petri/graph_models/star_forms.hpp"

#include <doctest/doctest.h>

using namespace petri;

namespace {

ReachabilityGraph sample_reachability_graph() {
    ReachabilityGraph reachability;
    reachability.initial_vertex_id = "m0";
    reachability.graph.add_vertex("m0");
    reachability.graph.add_vertex("m1");
    reachability.graph.add_vertex("m2");
    reachability.graph.add_edge(GraphEdge{"e1", "m0", "m1", "t0", 1.5});
    reachability.graph.add_edge(GraphEdge{"e2", "m0", "m2", "t1", 2.0});
    reachability.graph.add_edge(GraphEdge{"e3", "m1", "m2", "t2", 3.0});
    reachability.graph.add_edge(GraphEdge{"e4", "m2", "m0", "t3", 4.0});
    reachability.depths["m0"] = 0;
    reachability.depths["m1"] = 1;
    reachability.depths["m2"] = 1;
    return reachability;
}

} // namespace

TEST_CASE("graph models convert reachability graph to adjacency list") {
    const auto reachability = sample_reachability_graph();

    const auto adjacency = adjacency_list_from_reachability_graph(reachability);

    REQUIRE_EQ(adjacency.vertex_ids.size(), 3);
    REQUIRE_EQ(adjacency.rows.size(), 3);
    CHECK_EQ(adjacency.vertex_ids[0], "m0");
    CHECK_EQ(adjacency.rows[0].vertex_id, "m0");
    REQUIRE_EQ(adjacency.rows[0].edges.size(), 2);
    CHECK_EQ(adjacency.rows[0].edges[0].edge_id, "e1");
    CHECK_EQ(adjacency.rows[0].edges[0].target_vertex_id, "m1");
    CHECK_EQ(adjacency.rows[0].edges[0].target_index, 1);
    CHECK_EQ(adjacency.rows[0].edges[1].target_vertex_id, "m2");
    CHECK_EQ(adjacency.rows[0].edges[1].target_index, 2);
}

TEST_CASE("graph models convert reachability graph to forward star form") {
    const auto reachability = sample_reachability_graph();

    const auto fsf = forward_star_from_reachability_graph(reachability);

    REQUIRE_EQ(fsf.vertex_ids.size(), 3);
    REQUIRE_EQ(fsf.row_offsets.size(), 4);
    CHECK_EQ(fsf.row_offsets[0], 0);
    CHECK_EQ(fsf.row_offsets[1], 2);
    CHECK_EQ(fsf.row_offsets[2], 3);
    CHECK_EQ(fsf.row_offsets[3], 4);

    REQUIRE_EQ(fsf.edge_ids.size(), 4);
    CHECK_EQ(fsf.edge_ids[0], "e1");
    CHECK_EQ(fsf.edge_ids[1], "e2");
    CHECK_EQ(fsf.edge_ids[2], "e3");
    CHECK_EQ(fsf.edge_ids[3], "e4");
    CHECK_EQ(fsf.target_vertex_ids[0], "m1");
    CHECK_EQ(fsf.target_indices[0], 1);
    CHECK_EQ(fsf.target_vertex_ids[3], "m0");
    CHECK_EQ(fsf.target_indices[3], 0);
    CHECK(fsf.weights[0] > 1.49);
    CHECK(fsf.weights[0] < 1.51);
}

TEST_CASE("graph models convert reachability graph to backward star form") {
    const auto reachability = sample_reachability_graph();

    const auto bsf = backward_star_from_reachability_graph(reachability);

    REQUIRE_EQ(bsf.vertex_ids.size(), 3);
    REQUIRE_EQ(bsf.row_offsets.size(), 4);
    CHECK_EQ(bsf.row_offsets[0], 0);
    CHECK_EQ(bsf.row_offsets[1], 1);
    CHECK_EQ(bsf.row_offsets[2], 2);
    CHECK_EQ(bsf.row_offsets[3], 4);

    REQUIRE_EQ(bsf.edge_ids.size(), 4);
    CHECK_EQ(bsf.edge_ids[0], "e4");
    CHECK_EQ(bsf.source_vertex_ids[0], "m2");
    CHECK_EQ(bsf.source_indices[0], 2);
    CHECK_EQ(bsf.edge_ids[1], "e1");
    CHECK_EQ(bsf.source_vertex_ids[1], "m0");
    CHECK_EQ(bsf.edge_ids[2], "e2");
    CHECK_EQ(bsf.edge_ids[3], "e3");
}

TEST_CASE("graph models serialize and deserialize adjacency list") {
    const auto reachability = sample_reachability_graph();
    const auto adjacency = adjacency_list_from_reachability_graph(reachability);

    auto parsed = adjacency_list_from_json(adjacency_list_to_json(adjacency));
    REQUIRE(parsed.has_value());

    REQUIRE_EQ(parsed.value().rows.size(), 3);
    REQUIRE_EQ(parsed.value().rows[0].edges.size(), 2);
    CHECK_EQ(parsed.value().rows[0].edges[0].edge_id, "e1");
    CHECK_EQ(parsed.value().rows[0].edges[0].target_index, 1);
}

TEST_CASE("graph models serialize and deserialize forward and backward star forms") {
    const auto reachability = sample_reachability_graph();
    const auto fsf = forward_star_from_reachability_graph(reachability);
    const auto bsf = backward_star_from_reachability_graph(reachability);

    auto parsed_fsf = forward_star_from_json(forward_star_to_json(fsf));
    auto parsed_bsf = backward_star_from_json(backward_star_to_json(bsf));
    REQUIRE(parsed_fsf.has_value());
    REQUIRE(parsed_bsf.has_value());

    REQUIRE_EQ(parsed_fsf.value().row_offsets.size(), 4);
    CHECK_EQ(parsed_fsf.value().row_offsets[1], 2);
    REQUIRE_EQ(parsed_fsf.value().target_indices.size(), 4);
    CHECK_EQ(parsed_fsf.value().target_indices[3], 0);

    REQUIRE_EQ(parsed_bsf.value().row_offsets.size(), 4);
    CHECK_EQ(parsed_bsf.value().row_offsets[2], 2);
    REQUIRE_EQ(parsed_bsf.value().source_indices.size(), 4);
    CHECK_EQ(parsed_bsf.value().source_indices[0], 2);
}
