#pragma once

#include "petri/common/result.hpp"
#include "petri/core_pn/model.hpp"

#include <nlohmann/json.hpp>

#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace petri {

class PetriNet {
public:
    static Result<PetriNet> from_json(const nlohmann::json& data);
    static Result<PetriNet> load_from_file(const std::string& path);

    const std::string& name() const {
        return name_;
    }

    const std::vector<Place>& places() const {
        return places_;
    }

    const std::vector<Transition>& transitions() const {
        return transitions_;
    }

    const std::vector<Arc>& arcs() const {
        return arcs_;
    }

    Marking initial_marking() const;
    nlohmann::json marking_to_json(const Marking& marking) const;

    std::vector<std::string> enabled_transitions(const Marking& marking) const;
    Result<bool> is_enabled(const Marking& marking, const std::string& transition_id) const;
    Result<Marking> fire_transition(const Marking& marking, const std::string& transition_id) const;

    bool has_place(const std::string& id) const;
    bool has_transition(const std::string& id) const;
    std::optional<std::size_t> place_index(const std::string& id) const;
    std::optional<std::size_t> transition_index(const std::string& id) const;
    double transition_fire_time(const std::string& transition_id) const;

    bool marking_matches(const Marking& marking, const nlohmann::json& partial_marking) const;

private:
    struct WeightedPlace {
        std::size_t place_index = 0;
        int weight = 1;
        std::string arc_id;
    };

    void rebuild_indices();

    std::string name_;
    std::vector<Place> places_;
    std::vector<Transition> transitions_;
    std::vector<Arc> arcs_;
    std::unordered_map<std::string, std::size_t> place_index_by_id_;
    std::unordered_map<std::string, std::size_t> transition_index_by_id_;
    std::vector<std::vector<WeightedPlace>> input_arcs_by_transition_;
    std::vector<std::vector<WeightedPlace>> output_arcs_by_transition_;
};

} // namespace petri
