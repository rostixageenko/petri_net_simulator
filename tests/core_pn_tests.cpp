#include "petri/core_pn/interpreter.hpp"
#include "petri/core_pn/petri_net.hpp"

#include <doctest/doctest.h>

using namespace petri;

TEST_CASE("loads mutex example and builds initial marking") {
    auto net = PetriNet::load_from_file("examples/mutex.json");
    REQUIRE(net.has_value());
    CHECK_EQ(net.value().places().size(), 7);
    CHECK_EQ(net.value().transitions().size(), 6);
    CHECK_EQ(net.value().arcs().size(), 16);

    const Marking marking = net.value().initial_marking();
    CHECK_EQ(net.value().marking_to_json(marking).at("p1_idle").get<int>(), 1);
    CHECK_EQ(net.value().marking_to_json(marking).at("p2_idle").get<int>(), 1);
    CHECK_EQ(net.value().marking_to_json(marking).at("mutex").get<int>(), 1);
}

TEST_CASE("validates arc direction and token counts") {
    const auto invalid_direction = nlohmann::json::parse(R"({
        "places": [{"id": "p1", "tokens": 1}, {"id": "p2", "tokens": 0}],
        "transitions": [{"id": "t1"}],
        "arcs": [{"id": "a1", "source": "p1", "target": "p2", "weight": 1}]
    })");
    auto direction_result = PetriNet::from_json(invalid_direction);
    REQUIRE(!direction_result.has_value());
    CHECK_EQ(direction_result.error().code, "INVALID_ARC_DIRECTION");

    const auto invalid_tokens = nlohmann::json::parse(R"({
        "places": [{"id": "p1", "tokens": -1}],
        "transitions": [{"id": "t1"}],
        "arcs": []
    })");
    auto token_result = PetriNet::from_json(invalid_tokens);
    REQUIRE(!token_result.has_value());
    CHECK_EQ(token_result.error().code, "INVALID_TOKEN_COUNT");
}

TEST_CASE("finds enabled transitions in mutex net") {
    auto net = PetriNet::load_from_file("examples/mutex.json");
    REQUIRE(net.has_value());

    const auto enabled = net.value().enabled_transitions(net.value().initial_marking());
    REQUIRE_EQ(enabled.size(), 2);
    CHECK_EQ(enabled[0], "t1_request");
    CHECK_EQ(enabled[1], "t2_request");

    auto t1_enter = net.value().is_enabled(net.value().initial_marking(), "t1_enter");
    REQUIRE(t1_enter.has_value());
    CHECK(!t1_enter.value());
}

TEST_CASE("fires one transition atomically") {
    auto net = PetriNet::load_from_file("examples/mutex.json");
    REQUIRE(net.has_value());

    auto next = net.value().fire_transition(net.value().initial_marking(), "t1_request");
    REQUIRE(next.has_value());

    const auto marking = net.value().marking_to_json(next.value());
    CHECK_EQ(marking.at("p1_idle").get<int>(), 0);
    CHECK_EQ(marking.at("p1_wait").get<int>(), 1);
    CHECK_EQ(marking.at("mutex").get<int>(), 1);

    auto enabled = net.value().enabled_transitions(next.value());
    REQUIRE_EQ(enabled.size(), 2);
    CHECK_EQ(enabled[0], "t1_enter");
    CHECK_EQ(enabled[1], "t2_request");
}

TEST_CASE("interpreter records step events") {
    auto net = PetriNet::load_from_file("examples/mutex.json");
    REQUIRE(net.has_value());

    Interpreter interpreter(net.value());
    auto event = interpreter.step("t1_request", SimulationStrategy::ById);
    REQUIRE(event.has_value());

    CHECK_EQ(event.value().step, 1);
    CHECK_EQ(event.value().fired_transition, "t1_request");
    CHECK_EQ(net.value().marking_to_json(event.value().marking_after).at("p1_wait").get<int>(), 1);
    CHECK_EQ(event.value().enabled_before.size(), 2);
    CHECK_EQ(event.value().enabled_after.size(), 2);
}

TEST_CASE("interpreter runs automatic simulation") {
    auto net = PetriNet::load_from_file("examples/mutex.json");
    REQUIRE(net.has_value());

    Interpreter interpreter(net.value());
    SimulationParams params;
    params.max_steps = 4;
    params.strategy = SimulationStrategy::RoundRobin;
    auto run = interpreter.run(params);
    REQUIRE(run.has_value());

    CHECK_EQ(run.value().steps_executed, 4);
    CHECK_EQ(run.value().events.size(), 4);
    CHECK(!run.value().deadlock);
}
