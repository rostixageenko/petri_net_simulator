#pragma once

#include "petri/runtime/algorithm_runtime.hpp"

#include <nlohmann/json.hpp>

#include <optional>
#include <string>
#include <vector>

namespace petri {

struct SelectionWeights {
    double elapsed_ms = 0.0;
    double path_length = 0.0;
    double path_cost = 0.0;
    double visited_vertices = 0.0;
};

struct AlgorithmCandidateReport {
    std::string algorithm;
    std::string status;
    bool found = false;
    bool eligible = false;
    double score = 0.0;
    AlgorithmMetrics metrics;
    std::string error_code;
    std::string error_message;
};

struct AlgorithmSelectionRequest {
    AlgorithmTask task;
    AlgorithmParams params;
    std::vector<std::string> algorithms;
    SelectionWeights weights;
    bool require_found = true;
};

struct AlgorithmSelectionReport {
    SelectionWeights weights;
    std::vector<AlgorithmCandidateReport> candidates;
    std::optional<std::string> best_algorithm;
    std::optional<AlgorithmResult> best_result;
};

double score_algorithm_result(const AlgorithmResult& result, const SelectionWeights& weights);

Result<AlgorithmSelectionReport> select_algorithm(const AlgorithmRegistry& registry,
                                                  const AlgorithmSelectionRequest& request);
Result<AlgorithmSelectionReport> select_algorithm(const AlgorithmSelectionRequest& request);

nlohmann::json algorithm_selection_report_to_json(const AlgorithmSelectionReport& report);

} // namespace petri
