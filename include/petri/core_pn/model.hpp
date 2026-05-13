#pragma once

#include <string>
#include <vector>

namespace petri {

struct Place {
    std::string id;
    std::string label;
    int tokens = 0;
};

struct Transition {
    std::string id;
    std::string label;
    double fire_time = 1.0;
};

enum class ArcDirection {
    PlaceToTransition,
    TransitionToPlace
};

struct Arc {
    std::string id;
    std::string source;
    std::string target;
    int weight = 1;
    ArcDirection direction = ArcDirection::PlaceToTransition;
};

class Marking {
public:
    Marking() = default;
    explicit Marking(std::vector<int> tokens) : tokens_(std::move(tokens)) {}

    const std::vector<int>& tokens() const {
        return tokens_;
    }

    std::vector<int>& tokens() {
        return tokens_;
    }

    int at(std::size_t index) const {
        return tokens_.at(index);
    }

    void set(std::size_t index, int value) {
        tokens_.at(index) = value;
    }

    std::size_t size() const {
        return tokens_.size();
    }

    std::string key() const {
        std::string result;
        for (std::size_t i = 0; i < tokens_.size(); ++i) {
            if (i != 0) {
                result += '|';
            }
            result += std::to_string(tokens_[i]);
        }
        return result;
    }

    bool operator==(const Marking& other) const {
        return tokens_ == other.tokens_;
    }

private:
    std::vector<int> tokens_;
};

struct SimulationEvent {
    std::size_t step = 0;
    double time = 0.0;
    std::string fired_transition;
    Marking marking_before;
    Marking marking_after;
    std::vector<std::string> enabled_before;
    std::vector<std::string> enabled_after;
};

} // namespace petri
