#pragma once

#include "petri/algorithms/algorithms.hpp"

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace petri {

struct AlgorithmRunContext {
    std::string request_id;
    std::string task_type = "graph";
    std::size_t place_count = 0;
    std::size_t transition_count = 0;
    std::size_t arc_count = 0;
    std::size_t built_state_count = 0;
    bool log_metrics = false;
    std::string metrics_log_path;
};

struct AlgorithmTask {
    std::string algorithm_name;
    const DirectedGraph* graph = nullptr;
    AlgorithmRunContext context;
};

class Algorithm {
public:
    virtual ~Algorithm() = default;

    virtual const std::string& name() const = 0;
    virtual Result<AlgorithmResult> run(const DirectedGraph& graph, const AlgorithmParams& params) const = 0;
};

using AlgorithmRunner = std::function<Result<AlgorithmResult>(const DirectedGraph&, const AlgorithmParams&)>;

class AlgorithmRegistry {
public:
    Result<void> register_algorithm(std::shared_ptr<Algorithm> algorithm);
    Result<void> register_algorithm(std::string name, AlgorithmRunner runner);

    bool has_algorithm(const std::string& name) const;
    std::vector<std::string> available_algorithms() const;

    Result<AlgorithmResult> run(const AlgorithmTask& task, const AlgorithmParams& params) const;

private:
    std::map<std::string, std::shared_ptr<Algorithm>> algorithms_;
};

AlgorithmRegistry create_builtin_algorithm_registry();
std::vector<std::string> available_algorithms();
Result<AlgorithmResult> run(const AlgorithmTask& task, const AlgorithmParams& params);

Result<AlgorithmResult> run_algorithm(const std::string& name, const DirectedGraph& graph,
                                      const AlgorithmParams& params);

} // namespace petri
