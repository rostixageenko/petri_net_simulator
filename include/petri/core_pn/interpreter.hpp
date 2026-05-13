#pragma once

#include "petri/common/result.hpp"
#include "petri/core_pn/model.hpp"
#include "petri/core_pn/petri_net.hpp"

#include <optional>
#include <string>
#include <vector>

namespace petri {

enum class SimulationStrategy {
    FirstEnabled,
    ById,
    RoundRobin
};

struct SimulationParams {
    std::size_t max_steps = 1;
    SimulationStrategy strategy = SimulationStrategy::FirstEnabled;
    bool stop_on_deadlock = true;
    bool return_events = true;
    std::optional<std::string> transition_id;
};

struct SimulationRun {
    Marking initial_marking;
    Marking final_marking;
    std::vector<SimulationEvent> events;
    std::size_t steps_executed = 0;
    double elapsed_ms = 0.0;
    bool deadlock = false;
};

Result<SimulationStrategy> parse_strategy(const std::string& value);
std::string strategy_to_string(SimulationStrategy strategy);

class Interpreter {
public:
    explicit Interpreter(const PetriNet& net);
    Interpreter(const PetriNet& net, Marking marking);

    const Marking& marking() const {
        return marking_;
    }

    double current_time() const {
        return current_time_;
    }

    Result<SimulationEvent> step(std::optional<std::string> transition_id = std::nullopt,
                                 SimulationStrategy strategy = SimulationStrategy::FirstEnabled);
    Result<SimulationRun> run(const SimulationParams& params);

private:
    Result<std::string> choose_transition(const std::vector<std::string>& enabled,
                                          std::optional<std::string> requested_transition,
                                          SimulationStrategy strategy);

    const PetriNet& net_;
    Marking marking_;
    std::size_t step_index_ = 0;
    double current_time_ = 0.0;
    std::size_t round_robin_cursor_ = 0;
};

} // namespace petri
