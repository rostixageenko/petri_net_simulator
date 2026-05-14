#include "petri/runtime/algorithm_runtime.hpp"

#include "petri/logging_metrics/metrics_logger.hpp"

#include <utility>

namespace petri {
namespace {

class FunctionAlgorithm final : public Algorithm {
public:
    FunctionAlgorithm(std::string name, AlgorithmRunner runner)
        : name_(std::move(name)), runner_(std::move(runner)) {}

    const std::string& name() const override {
        return name_;
    }

    Result<AlgorithmResult> run(const DirectedGraph& graph, const AlgorithmParams& params) const override {
        return runner_(graph, params);
    }

private:
    std::string name_;
    AlgorithmRunner runner_;
};

std::size_t built_states_for(const AlgorithmTask& task) {
    if (task.context.built_state_count != 0) {
        return task.context.built_state_count;
    }
    if (task.graph != nullptr) {
        return task.graph->vertex_count();
    }
    return 0;
}

void write_metrics(const AlgorithmTask& task, const AlgorithmResult& result, const std::string& error_message) {
    if (!task.context.log_metrics) {
        return;
    }

    const auto record = create_metrics_record(RunMetricsInput{
        task.context.request_id,
        result.algorithm.empty() ? task.algorithm_name : result.algorithm,
        task.context.task_type,
        task.context.place_count,
        task.context.transition_count,
        task.context.arc_count,
        built_states_for(task),
        result.metrics.scanned_edges,
        result.metrics.elapsed_ms,
        result.found,
        result.metrics.path_cost,
        result.metrics.path_length,
        error_message,
    });

    if (task.context.metrics_log_path.empty()) {
        (void)append_metrics_record(record);
    } else {
        (void)append_metrics_record(record, task.context.metrics_log_path);
    }
}

void write_error_metrics(const AlgorithmTask& task, const std::string& error_message) {
    if (!task.context.log_metrics) {
        return;
    }

    AlgorithmResult result;
    result.algorithm = task.algorithm_name;
    result.status = "error";
    result.found = false;
    write_metrics(task, result, error_message);
}

Result<void> register_builtin_algorithms(AlgorithmRegistry& registry) {
    auto dfs = registry.register_algorithm("dfs", run_dfs);
    if (!dfs) {
        return dfs;
    }
    auto bfs = registry.register_algorithm("bfs", run_bfs);
    if (!bfs) {
        return bfs;
    }
    return registry.register_algorithm("dijkstra", run_dijkstra);
}

} // namespace

Result<void> AlgorithmRegistry::register_algorithm(std::shared_ptr<Algorithm> algorithm) {
    if (!algorithm) {
        return Result<void>::failure(
            make_error("INVALID_ALGORITHM", "Algorithm implementation must not be null"));
    }
    if (algorithm->name().empty()) {
        return Result<void>::failure(
            make_error("INVALID_ALGORITHM", "Algorithm name must not be empty"));
    }
    if (algorithms_.find(algorithm->name()) != algorithms_.end()) {
        return Result<void>::failure(
            make_error("DUPLICATE_ID", "Algorithm is already registered", {{"algorithm", algorithm->name()}}));
    }

    algorithms_[algorithm->name()] = std::move(algorithm);
    return Result<void>::success();
}

Result<void> AlgorithmRegistry::register_algorithm(std::string name, AlgorithmRunner runner) {
    if (!runner) {
        return Result<void>::failure(
            make_error("INVALID_ALGORITHM", "Algorithm runner must not be empty", {{"algorithm", name}}));
    }
    return register_algorithm(std::make_shared<FunctionAlgorithm>(std::move(name), std::move(runner)));
}

bool AlgorithmRegistry::has_algorithm(const std::string& name) const {
    return algorithms_.find(name) != algorithms_.end();
}

std::vector<std::string> AlgorithmRegistry::available_algorithms() const {
    std::vector<std::string> names;
    names.reserve(algorithms_.size());
    for (const auto& [name, algorithm] : algorithms_) {
        (void)algorithm;
        names.push_back(name);
    }
    return names;
}

Result<AlgorithmResult> AlgorithmRegistry::run(const AlgorithmTask& task, const AlgorithmParams& params) const {
    if (task.graph == nullptr) {
        const auto error = make_error("INVALID_ALGORITHM_TASK", "Algorithm task graph must not be null");
        write_error_metrics(task, error.message);
        return Result<AlgorithmResult>::failure(error);
    }

    const auto it = algorithms_.find(task.algorithm_name);
    if (it == algorithms_.end()) {
        const auto error = make_error(
            "UNKNOWN_ALGORITHM", "Unknown algorithm", {{"algorithm", task.algorithm_name}});
        write_error_metrics(task, error.message);
        return Result<AlgorithmResult>::failure(error);
    }

    auto result = it->second->run(*task.graph, params);
    if (!result) {
        write_error_metrics(task, result.error().message);
        return result;
    }

    std::string error_message;
    if (!result.value().found && params.target_vertex_id) {
        error_message = "Target not found";
    }
    write_metrics(task, result.value(), error_message);
    return result;
}

AlgorithmRegistry create_builtin_algorithm_registry() {
    AlgorithmRegistry registry;
    (void)register_builtin_algorithms(registry);
    return registry;
}

std::vector<std::string> available_algorithms() {
    return create_builtin_algorithm_registry().available_algorithms();
}

Result<AlgorithmResult> run(const AlgorithmTask& task, const AlgorithmParams& params) {
    return create_builtin_algorithm_registry().run(task, params);
}

Result<AlgorithmResult> run_algorithm(const std::string& name, const DirectedGraph& graph,
                                      const AlgorithmParams& params) {
    AlgorithmTask task;
    task.algorithm_name = name;
    task.graph = &graph;
    return run(task, params);
}

} // namespace petri
