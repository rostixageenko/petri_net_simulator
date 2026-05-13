#include "petri/core_pn/petri_net.hpp"

#include <fstream>
#include <set>
#include <sstream>

namespace petri {
namespace {

bool is_non_negative_integer(const nlohmann::json& value) {
    return value.is_number_integer() && value.get<int>() >= 0;
}

bool is_positive_integer(const nlohmann::json& value) {
    return value.is_number_integer() && value.get<int>() > 0;
}

Result<std::string> required_string(const nlohmann::json& object, const std::string& field,
                                    const std::string& code) {
    if (!object.contains(field) || !object.at(field).is_string()) {
        return Result<std::string>::failure(make_error(
            code, "Expected string field '" + field + "'", {{"field", field}}));
    }
    return Result<std::string>::success(object.at(field).get<std::string>());
}

std::string optional_label(const nlohmann::json& object, const std::string& id) {
    if (object.contains("label") && object.at("label").is_string()) {
        return object.at("label").get<std::string>();
    }
    return id;
}

} // namespace

Result<PetriNet> PetriNet::load_from_file(const std::string& path) {
    std::ifstream input(path);
    if (!input) {
        return Result<PetriNet>::failure(
            make_error("INVALID_JSON", "Cannot open JSON file", {{"path", path}}));
    }

    try {
        nlohmann::json data;
        input >> data;
        return from_json(data);
    } catch (const std::exception& ex) {
        return Result<PetriNet>::failure(make_error("INVALID_JSON", ex.what(), {{"path", path}}));
    }
}

Result<PetriNet> PetriNet::from_json(const nlohmann::json& data) {
    if (!data.is_object()) {
        return Result<PetriNet>::failure(make_error("INVALID_JSON", "Root JSON value must be an object"));
    }
    if (!data.contains("places") || !data.at("places").is_array()) {
        return Result<PetriNet>::failure(make_error("INVALID_JSON", "Field 'places' must be an array"));
    }
    if (!data.contains("transitions") || !data.at("transitions").is_array()) {
        return Result<PetriNet>::failure(make_error("INVALID_JSON", "Field 'transitions' must be an array"));
    }
    if (!data.contains("arcs") || !data.at("arcs").is_array()) {
        return Result<PetriNet>::failure(make_error("INVALID_JSON", "Field 'arcs' must be an array"));
    }

    PetriNet net;
    net.name_ = data.value<std::string>("name", "Petri net");

    std::set<std::string> place_ids;
    for (const auto& place_json : data.at("places")) {
        if (!place_json.is_object()) {
            return Result<PetriNet>::failure(make_error("INVALID_JSON", "Place entry must be an object"));
        }

        auto id_result = required_string(place_json, "id", "INVALID_JSON");
        if (!id_result) {
            return Result<PetriNet>::failure(id_result.error());
        }
        const std::string id = id_result.value();
        if (id.empty() || place_ids.contains(id)) {
            return Result<PetriNet>::failure(
                make_error("DUPLICATE_ID", "Duplicate or empty place id", {{"id", id}}));
        }
        place_ids.insert(id);

        int tokens = 0;
        if (place_json.contains("tokens")) {
            if (!is_non_negative_integer(place_json.at("tokens"))) {
                return Result<PetriNet>::failure(make_error(
                    "INVALID_TOKEN_COUNT", "Place tokens must be a non-negative integer", {{"place_id", id}}));
            }
            tokens = place_json.at("tokens").get<int>();
        }

        net.places_.push_back(Place{id, optional_label(place_json, id), tokens});
    }

    std::set<std::string> transition_ids;
    for (const auto& transition_json : data.at("transitions")) {
        if (!transition_json.is_object()) {
            return Result<PetriNet>::failure(make_error("INVALID_JSON", "Transition entry must be an object"));
        }

        auto id_result = required_string(transition_json, "id", "INVALID_JSON");
        if (!id_result) {
            return Result<PetriNet>::failure(id_result.error());
        }
        const std::string id = id_result.value();
        if (id.empty() || transition_ids.contains(id)) {
            return Result<PetriNet>::failure(
                make_error("DUPLICATE_ID", "Duplicate or empty transition id", {{"id", id}}));
        }
        transition_ids.insert(id);

        double fire_time = 1.0;
        if (transition_json.contains("fire_time")) {
            if (!transition_json.at("fire_time").is_number() || transition_json.at("fire_time").get<double>() < 0.0) {
                return Result<PetriNet>::failure(make_error(
                    "INVALID_JSON", "Transition fire_time must be a non-negative number", {{"transition_id", id}}));
            }
            fire_time = transition_json.at("fire_time").get<double>();
        }

        net.transitions_.push_back(Transition{id, optional_label(transition_json, id), fire_time});
    }

    net.rebuild_indices();

    std::set<std::string> arc_ids;
    for (const auto& arc_json : data.at("arcs")) {
        if (!arc_json.is_object()) {
            return Result<PetriNet>::failure(make_error("INVALID_JSON", "Arc entry must be an object"));
        }

        auto id_result = required_string(arc_json, "id", "INVALID_JSON");
        auto source_result = required_string(arc_json, "source", "INVALID_ARC_ENDPOINT");
        auto target_result = required_string(arc_json, "target", "INVALID_ARC_ENDPOINT");
        if (!id_result) {
            return Result<PetriNet>::failure(id_result.error());
        }
        if (!source_result) {
            return Result<PetriNet>::failure(source_result.error());
        }
        if (!target_result) {
            return Result<PetriNet>::failure(target_result.error());
        }

        const std::string id = id_result.value();
        const std::string source = source_result.value();
        const std::string target = target_result.value();
        if (id.empty() || arc_ids.contains(id)) {
            return Result<PetriNet>::failure(
                make_error("DUPLICATE_ID", "Duplicate or empty arc id", {{"id", id}}));
        }
        arc_ids.insert(id);

        if (!arc_json.contains("weight") || !is_positive_integer(arc_json.at("weight"))) {
            return Result<PetriNet>::failure(
                make_error("INVALID_ARC_WEIGHT", "Arc weight must be a positive integer", {{"arc_id", id}}));
        }
        const int weight = arc_json.at("weight").get<int>();

        const bool source_is_place = net.has_place(source);
        const bool source_is_transition = net.has_transition(source);
        const bool target_is_place = net.has_place(target);
        const bool target_is_transition = net.has_transition(target);

        if ((!source_is_place && !source_is_transition) || (!target_is_place && !target_is_transition)) {
            return Result<PetriNet>::failure(make_error(
                "UNKNOWN_NODE_ID", "Arc references an unknown node", {{"arc_id", id}, {"source", source}, {"target", target}}));
        }

        if (source_is_place && target_is_transition) {
            net.arcs_.push_back(Arc{id, source, target, weight, ArcDirection::PlaceToTransition});
        } else if (source_is_transition && target_is_place) {
            net.arcs_.push_back(Arc{id, source, target, weight, ArcDirection::TransitionToPlace});
        } else {
            return Result<PetriNet>::failure(make_error(
                "INVALID_ARC_DIRECTION", "Arc must connect a place and a transition", {{"arc_id", id}}));
        }
    }

    net.rebuild_indices();
    return Result<PetriNet>::success(std::move(net));
}

void PetriNet::rebuild_indices() {
    place_index_by_id_.clear();
    transition_index_by_id_.clear();

    for (std::size_t i = 0; i < places_.size(); ++i) {
        place_index_by_id_[places_[i].id] = i;
    }
    for (std::size_t i = 0; i < transitions_.size(); ++i) {
        transition_index_by_id_[transitions_[i].id] = i;
    }

    input_arcs_by_transition_.assign(transitions_.size(), {});
    output_arcs_by_transition_.assign(transitions_.size(), {});
    for (const auto& arc : arcs_) {
        if (arc.direction == ArcDirection::PlaceToTransition) {
            const auto place = place_index(arc.source);
            const auto transition = transition_index(arc.target);
            if (place && transition) {
                input_arcs_by_transition_[*transition].push_back(WeightedPlace{*place, arc.weight, arc.id});
            }
        } else {
            const auto transition = transition_index(arc.source);
            const auto place = place_index(arc.target);
            if (place && transition) {
                output_arcs_by_transition_[*transition].push_back(WeightedPlace{*place, arc.weight, arc.id});
            }
        }
    }
}

Marking PetriNet::initial_marking() const {
    std::vector<int> tokens;
    tokens.reserve(places_.size());
    for (const auto& place : places_) {
        tokens.push_back(place.tokens);
    }
    return Marking(std::move(tokens));
}

nlohmann::json PetriNet::marking_to_json(const Marking& marking) const {
    nlohmann::json result = nlohmann::json::object();
    for (std::size_t i = 0; i < places_.size() && i < marking.size(); ++i) {
        result[places_[i].id] = marking.at(i);
    }
    return result;
}

std::vector<std::string> PetriNet::enabled_transitions(const Marking& marking) const {
    std::vector<std::string> enabled;
    for (std::size_t transition_index = 0; transition_index < transitions_.size(); ++transition_index) {
        bool can_fire = true;
        for (const auto& input : input_arcs_by_transition_[transition_index]) {
            if (input.place_index >= marking.size() || marking.at(input.place_index) < input.weight) {
                can_fire = false;
                break;
            }
        }
        if (can_fire) {
            enabled.push_back(transitions_[transition_index].id);
        }
    }
    return enabled;
}

Result<bool> PetriNet::is_enabled(const Marking& marking, const std::string& transition_id) const {
    const auto index = transition_index(transition_id);
    if (!index) {
        return Result<bool>::failure(make_error(
            "UNKNOWN_TRANSITION_ID", "Unknown transition id", {{"transition_id", transition_id}}));
    }

    for (const auto& input : input_arcs_by_transition_[*index]) {
        if (input.place_index >= marking.size() || marking.at(input.place_index) < input.weight) {
            return Result<bool>::success(false);
        }
    }
    return Result<bool>::success(true);
}

Result<Marking> PetriNet::fire_transition(const Marking& marking, const std::string& transition_id) const {
    const auto index = transition_index(transition_id);
    if (!index) {
        return Result<Marking>::failure(make_error(
            "UNKNOWN_TRANSITION_ID", "Unknown transition id", {{"transition_id", transition_id}}));
    }

    auto enabled = is_enabled(marking, transition_id);
    if (!enabled) {
        return Result<Marking>::failure(enabled.error());
    }
    if (!enabled.value()) {
        return Result<Marking>::failure(make_error(
            "TRANSITION_NOT_ENABLED", "Transition is not enabled", {{"transition_id", transition_id}}));
    }

    Marking next = marking;
    for (const auto& input : input_arcs_by_transition_[*index]) {
        next.set(input.place_index, next.at(input.place_index) - input.weight);
    }
    for (const auto& output : output_arcs_by_transition_[*index]) {
        next.set(output.place_index, next.at(output.place_index) + output.weight);
    }
    return Result<Marking>::success(std::move(next));
}

bool PetriNet::has_place(const std::string& id) const {
    return place_index_by_id_.find(id) != place_index_by_id_.end();
}

bool PetriNet::has_transition(const std::string& id) const {
    return transition_index_by_id_.find(id) != transition_index_by_id_.end();
}

std::optional<std::size_t> PetriNet::place_index(const std::string& id) const {
    const auto it = place_index_by_id_.find(id);
    if (it == place_index_by_id_.end()) {
        return std::nullopt;
    }
    return it->second;
}

std::optional<std::size_t> PetriNet::transition_index(const std::string& id) const {
    const auto it = transition_index_by_id_.find(id);
    if (it == transition_index_by_id_.end()) {
        return std::nullopt;
    }
    return it->second;
}

double PetriNet::transition_fire_time(const std::string& transition_id) const {
    const auto index = transition_index(transition_id);
    if (!index) {
        return 1.0;
    }
    return transitions_[*index].fire_time;
}

bool PetriNet::marking_matches(const Marking& marking, const nlohmann::json& partial_marking) const {
    if (!partial_marking.is_object()) {
        return false;
    }

    for (const auto& place : places_) {
        if (!partial_marking.contains(place.id)) {
            continue;
        }
        const auto index = place_index(place.id);
        if (!index || !partial_marking.at(place.id).is_number_integer()) {
            return false;
        }
        if (marking.at(*index) != partial_marking.at(place.id).get<int>()) {
            return false;
        }
    }
    return true;
}

} // namespace petri
