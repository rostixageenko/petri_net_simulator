#include "petri/core_pn/interpreter.hpp"

#include <algorithm>
#include <chrono>

namespace petri {

Result<SimulationStrategy> parse_strategy(const std::string& value) {
    if (value == "first_enabled") {
        return Result<SimulationStrategy>::success(SimulationStrategy::FirstEnabled);
    }
    if (value == "by_id") {
        return Result<SimulationStrategy>::success(SimulationStrategy::ById);
    }
    if (value == "round_robin") {
        return Result<SimulationStrategy>::success(SimulationStrategy::RoundRobin);
    }
    return Result<SimulationStrategy>::failure(
        make_error("INVALID_JSON", "Unknown simulation strategy", {{"strategy", value}}));
}

std::string strategy_to_string(SimulationStrategy strategy) {
    switch (strategy) {
        case SimulationStrategy::FirstEnabled:
            return "first_enabled";
        case SimulationStrategy::ById:
            return "by_id";
        case SimulationStrategy::RoundRobin:
            return "round_robin";
    }
    return "first_enabled";
}

Interpreter::Interpreter(const PetriNet& net) : net_(net), marking_(net.initial_marking()) {}

Interpreter::Interpreter(const PetriNet& net, Marking marking) : net_(net), marking_(std::move(marking)) {}

Result<std::string> Interpreter::choose_transition(const std::vector<std::string>& enabled,
                                                   std::optional<std::string> requested_transition,
                                                   SimulationStrategy strategy) {
    if (enabled.empty()) {
        return Result<std::string>::failure(make_error("TRANSITION_NOT_ENABLED", "No enabled transitions"));
    }

    if (requested_transition || strategy == SimulationStrategy::ById) {
        if (!requested_transition) {
            return Result<std::string>::failure(
                make_error("UNKNOWN_TRANSITION_ID", "Strategy by_id requires transition_id"));
        }
        const auto it = std::find(enabled.begin(), enabled.end(), *requested_transition);
        if (it == enabled.end()) {
            return Result<std::string>::failure(make_error(
                "TRANSITION_NOT_ENABLED", "Requested transition is not enabled", {{"transition_id", *requested_transition}}));
        }
        return Result<std::string>::success(*requested_transition);
    }

    if (strategy == SimulationStrategy::RoundRobin) {
        for (std::size_t offset = 0; offset < net_.transitions().size(); ++offset) {
            const std::size_t candidate_index = (round_robin_cursor_ + offset) % net_.transitions().size();
            const std::string& candidate = net_.transitions()[candidate_index].id;
            if (std::find(enabled.begin(), enabled.end(), candidate) != enabled.end()) {
                round_robin_cursor_ = (candidate_index + 1) % net_.transitions().size();
                return Result<std::string>::success(candidate);
            }
        }
    }

    return Result<std::string>::success(enabled.front());
}

Result<SimulationEvent> Interpreter::step(std::optional<std::string> transition_id, SimulationStrategy strategy) {
    const auto before = marking_;
    const auto enabled_before = net_.enabled_transitions(before);
    auto chosen = choose_transition(enabled_before, std::move(transition_id), strategy);
    if (!chosen) {
        return Result<SimulationEvent>::failure(chosen.error());
    }

    auto next = net_.fire_transition(before, chosen.value());
    if (!next) {
        return Result<SimulationEvent>::failure(next.error());
    }

    current_time_ += net_.transition_fire_time(chosen.value());
    marking_ = next.value();
    ++step_index_;

    SimulationEvent event;
    event.step = step_index_;
    event.time = current_time_;
    event.fired_transition = chosen.value();
    event.marking_before = before;
    event.marking_after = marking_;
    event.enabled_before = enabled_before;
    event.enabled_after = net_.enabled_transitions(marking_);
    return Result<SimulationEvent>::success(std::move(event));
}

Result<SimulationRun> Interpreter::run(const SimulationParams& params) {
    using Clock = std::chrono::steady_clock;
    const auto started_at = Clock::now();

    SimulationRun run;
    run.initial_marking = marking_;

    for (std::size_t i = 0; i < params.max_steps; ++i) {
        if (net_.enabled_transitions(marking_).empty()) {
            run.deadlock = true;
            if (params.stop_on_deadlock) {
                break;
            }
        }

        auto event = step(params.transition_id, params.strategy);
        if (!event) {
            if (event.error().code == "TRANSITION_NOT_ENABLED" && params.stop_on_deadlock) {
                run.deadlock = true;
                break;
            }
            return Result<SimulationRun>::failure(event.error());
        }

        ++run.steps_executed;
        if (params.return_events) {
            run.events.push_back(event.value());
        }
    }

    run.final_marking = marking_;
    const auto finished_at = Clock::now();
    run.elapsed_ms = std::chrono::duration<double, std::milli>(finished_at - started_at).count();
    return Result<SimulationRun>::success(std::move(run));
}

} // namespace petri
