#include "petri/algorithm_selector/algorithm_selector.hpp"

#include <limits>
#include <utility>

namespace petri {
namespace {

nlohmann::json weights_to_json(const SelectionWeights& weights) {
    nlohmann::json result = nlohmann::json::object();
    result["elapsed_ms"] = weights.elapsed_ms;
    result["path_length"] = weights.path_length;
    result["path_cost"] = weights.path_cost;
    result["visited_vertices"] = weights.visited_vertices;
    return result;
}

nlohmann::json metrics_to_json(const AlgorithmMetrics& metrics) {
    nlohmann::json result = nlohmann::json::object();
    result["elapsed_ms"] = metrics.elapsed_ms;
    result["visited_vertices"] = metrics.visited_vertices;
    result["scanned_edges"] = metrics.scanned_edges;
    result["path_length"] = metrics.path_length;
    result["path_cost"] = metrics.path_cost;
    return result;
}

nlohmann::json result_to_json(const AlgorithmResult& result) {
    nlohmann::json data = nlohmann::json::object();
    data["algorithm"] = result.algorithm;
    data["status"] = result.status;
    data["found"] = result.found;
    data["path"] = nlohmann::json::array();
    for (const auto& vertex_id : result.path) {
        data["path"].push_back(vertex_id);
    }
    data["metrics"] = metrics_to_json(result.metrics);
    return data;
}

nlohmann::json candidate_to_json(const AlgorithmCandidateReport& candidate) {
    nlohmann::json data = nlohmann::json::object();
    data["algorithm"] = candidate.algorithm;
    data["status"] = candidate.status;
    data["found"] = candidate.found;
    data["eligible"] = candidate.eligible;
    data["score"] = candidate.score;
    data["metrics"] = metrics_to_json(candidate.metrics);
    data["error_message"] = candidate.error_message;
    return data;
}

} // namespace

double score_algorithm_result(const AlgorithmResult& result, const SelectionWeights& weights) {
    return weights.elapsed_ms * result.metrics.elapsed_ms +
           weights.path_length * static_cast<double>(result.metrics.path_length) +
           weights.path_cost * result.metrics.path_cost +
           weights.visited_vertices * static_cast<double>(result.metrics.visited_vertices);
}

Result<AlgorithmSelectionReport> select_algorithm(const AlgorithmRegistry& registry,
                                                  const AlgorithmSelectionRequest& request) {
    AlgorithmSelectionReport report;
    report.weights = request.weights;

    const std::vector<std::string> algorithms =
        request.algorithms.empty() ? registry.available_algorithms() : request.algorithms;

    double best_score = std::numeric_limits<double>::infinity();
    for (const auto& algorithm_name : algorithms) {
        AlgorithmTask task = request.task;
        task.algorithm_name = algorithm_name;

        AlgorithmCandidateReport candidate;
        candidate.algorithm = algorithm_name;

        auto run_result = registry.run(task, request.params);
        if (!run_result) {
            candidate.status = "error";
            candidate.error_message = run_result.error().message;
            report.candidates.push_back(std::move(candidate));
            continue;
        }

        const AlgorithmResult& result = run_result.value();
        candidate.status = result.status;
        candidate.found = result.found;
        candidate.metrics = result.metrics;
        candidate.eligible = !request.require_found || result.found;
        if (candidate.eligible) {
            candidate.score = score_algorithm_result(result, request.weights);
            if (!report.best_algorithm || candidate.score < best_score) {
                best_score = candidate.score;
                report.best_algorithm = algorithm_name;
                report.best_result = result;
            }
        }

        report.candidates.push_back(std::move(candidate));
    }

    return Result<AlgorithmSelectionReport>::success(std::move(report));
}

Result<AlgorithmSelectionReport> select_algorithm(const AlgorithmSelectionRequest& request) {
    return select_algorithm(create_builtin_algorithm_registry(), request);
}

nlohmann::json algorithm_selection_report_to_json(const AlgorithmSelectionReport& report) {
    nlohmann::json comparison = nlohmann::json::array();
    for (const auto& candidate : report.candidates) {
        comparison.push_back(candidate_to_json(candidate));
    }

    nlohmann::json response = nlohmann::json::object();
    response["best_algorithm"] = report.best_algorithm ? nlohmann::json(*report.best_algorithm) : nlohmann::json();
    response["criteria"] = weights_to_json(report.weights);
    response["comparison"] = std::move(comparison);

    if (report.best_result) {
        response["best_result"] = result_to_json(*report.best_result);
    }

    return response;
}

} // namespace petri
