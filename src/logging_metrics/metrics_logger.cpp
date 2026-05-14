#include "petri/logging_metrics/metrics_logger.hpp"

#include <filesystem>
#include <fstream>
#include <utility>

namespace petri {
namespace {

Result<std::string> read_string(const nlohmann::json& data, const std::string& field) {
    if (!data.contains(field) || !data.at(field).is_string()) {
        return Result<std::string>::failure(
            make_error("INVALID_JSON", "Metrics field must be a string", {{"field", field}}));
    }
    return Result<std::string>::success(data.at(field).get<std::string>());
}

Result<std::size_t> read_size(const nlohmann::json& data, const std::string& field) {
    if (!data.contains(field) || !data.at(field).is_number_integer()) {
        return Result<std::size_t>::failure(
            make_error("INVALID_JSON", "Metrics field must be an integer", {{"field", field}}));
    }
    const int value = data.at(field).get<int>();
    if (value < 0) {
        return Result<std::size_t>::failure(
            make_error("INVALID_JSON", "Metrics field must be non-negative", {{"field", field}}));
    }
    return Result<std::size_t>::success(static_cast<std::size_t>(value));
}

Result<double> read_double(const nlohmann::json& data, const std::string& field) {
    if (!data.contains(field) || !data.at(field).is_number()) {
        return Result<double>::failure(
            make_error("INVALID_JSON", "Metrics field must be a number", {{"field", field}}));
    }
    return Result<double>::success(data.at(field).get<double>());
}

Result<bool> read_bool(const nlohmann::json& data, const std::string& field) {
    if (!data.contains(field) || !data.at(field).is_boolean()) {
        return Result<bool>::failure(
            make_error("INVALID_JSON", "Metrics field must be a boolean", {{"field", field}}));
    }
    return Result<bool>::success(data.at(field).get<bool>());
}

Result<void> ensure_log_directory(const std::string& path) {
    try {
        const std::filesystem::path log_path(path);
        const auto parent = log_path.parent_path();
        if (!parent.empty()) {
            std::filesystem::create_directories(parent);
        }
        return Result<void>::success();
    } catch (const std::exception& ex) {
        return Result<void>::failure(
            make_error("METRICS_LOG_ERROR", ex.what(), {{"path", path}}));
    }
}

} // namespace

RunMetricsRecord create_metrics_record(RunMetricsInput input) {
    RunMetricsRecord record;
    record.request_id = std::move(input.request_id);
    record.algorithm_name = std::move(input.algorithm_name);
    record.task_type = std::move(input.task_type);
    record.place_count = input.place_count;
    record.transition_count = input.transition_count;
    record.arc_count = input.arc_count;
    record.built_state_count = input.built_state_count;
    record.scanned_edge_count = input.scanned_edge_count;
    record.elapsed_ms = input.elapsed_ms;
    record.found = input.found;
    record.path_cost = input.path_cost;
    record.path_length = input.path_length;
    record.error_message = std::move(input.error_message);
    return record;
}

nlohmann::json metrics_record_to_json(const RunMetricsRecord& record) {
    nlohmann::json result = nlohmann::json::object();
    result["request_id"] = record.request_id;
    result["algorithm"] = record.algorithm_name;
    result["task_type"] = record.task_type;
    result["places"] = record.place_count;
    result["transitions"] = record.transition_count;
    result["arcs"] = record.arc_count;
    result["built_states"] = record.built_state_count;
    result["scanned_edges"] = record.scanned_edge_count;
    result["elapsed_ms"] = record.elapsed_ms;
    result["found"] = record.found;
    result["path_cost"] = record.path_cost;
    result["path_length"] = record.path_length;
    result["error_message"] = record.error_message;
    return result;
}

Result<RunMetricsRecord> metrics_record_from_json(const nlohmann::json& data) {
    if (!data.is_object()) {
        return Result<RunMetricsRecord>::failure(
            make_error("INVALID_JSON", "Metrics record must be an object"));
    }

    auto request_id = read_string(data, "request_id");
    auto algorithm = read_string(data, "algorithm");
    auto task_type = read_string(data, "task_type");
    auto places = read_size(data, "places");
    auto transitions = read_size(data, "transitions");
    auto arcs = read_size(data, "arcs");
    auto built_states = read_size(data, "built_states");
    auto scanned_edges = read_size(data, "scanned_edges");
    auto elapsed = read_double(data, "elapsed_ms");
    auto found = read_bool(data, "found");
    auto path_cost = read_double(data, "path_cost");
    auto path_length = read_size(data, "path_length");
    auto error_message = read_string(data, "error_message");

    if (!request_id) {
        return Result<RunMetricsRecord>::failure(request_id.error());
    }
    if (!algorithm) {
        return Result<RunMetricsRecord>::failure(algorithm.error());
    }
    if (!task_type) {
        return Result<RunMetricsRecord>::failure(task_type.error());
    }
    if (!places) {
        return Result<RunMetricsRecord>::failure(places.error());
    }
    if (!transitions) {
        return Result<RunMetricsRecord>::failure(transitions.error());
    }
    if (!arcs) {
        return Result<RunMetricsRecord>::failure(arcs.error());
    }
    if (!built_states) {
        return Result<RunMetricsRecord>::failure(built_states.error());
    }
    if (!scanned_edges) {
        return Result<RunMetricsRecord>::failure(scanned_edges.error());
    }
    if (!elapsed) {
        return Result<RunMetricsRecord>::failure(elapsed.error());
    }
    if (!found) {
        return Result<RunMetricsRecord>::failure(found.error());
    }
    if (!path_cost) {
        return Result<RunMetricsRecord>::failure(path_cost.error());
    }
    if (!path_length) {
        return Result<RunMetricsRecord>::failure(path_length.error());
    }
    if (!error_message) {
        return Result<RunMetricsRecord>::failure(error_message.error());
    }

    return Result<RunMetricsRecord>::success(create_metrics_record(RunMetricsInput{
        request_id.value(),
        algorithm.value(),
        task_type.value(),
        places.value(),
        transitions.value(),
        arcs.value(),
        built_states.value(),
        scanned_edges.value(),
        elapsed.value(),
        found.value(),
        path_cost.value(),
        path_length.value(),
        error_message.value(),
    }));
}

std::string default_metrics_log_path() {
    return "logs/metrics.jsonl";
}

Result<void> append_metrics_record(const RunMetricsRecord& record) {
    return append_metrics_record(record, default_metrics_log_path());
}

Result<void> append_metrics_record(const RunMetricsRecord& record, const std::string& path) {
    auto directory = ensure_log_directory(path);
    if (!directory) {
        return directory;
    }

    std::ofstream output(path, std::ios::app);
    if (!output) {
        return Result<void>::failure(
            make_error("METRICS_LOG_ERROR", "Cannot open metrics log file", {{"path", path}}));
    }

    output << metrics_record_to_json(record).dump() << '\n';
    if (!output) {
        return Result<void>::failure(
            make_error("METRICS_LOG_ERROR", "Cannot write metrics log file", {{"path", path}}));
    }
    return Result<void>::success();
}

Result<std::vector<RunMetricsRecord>> read_metrics_records() {
    return read_metrics_records(default_metrics_log_path());
}

Result<std::vector<RunMetricsRecord>> read_metrics_records(const std::string& path) {
    std::ifstream input(path);
    if (!input) {
        return Result<std::vector<RunMetricsRecord>>::failure(
            make_error("METRICS_LOG_ERROR", "Cannot open metrics log file", {{"path", path}}));
    }

    std::vector<RunMetricsRecord> records;
    std::string line;
    std::size_t line_number = 0;
    while (std::getline(input, line)) {
        ++line_number;
        if (line.empty()) {
            continue;
        }

        try {
            auto parsed = metrics_record_from_json(nlohmann::json::parse(line));
            if (!parsed) {
                auto error = parsed.error();
                error.details["line"] = std::to_string(line_number);
                return Result<std::vector<RunMetricsRecord>>::failure(std::move(error));
            }
            records.push_back(parsed.value());
        } catch (const std::exception& ex) {
            return Result<std::vector<RunMetricsRecord>>::failure(make_error(
                "INVALID_JSON", ex.what(), {{"path", path}, {"line", std::to_string(line_number)}}));
        }
    }

    return Result<std::vector<RunMetricsRecord>>::success(std::move(records));
}

} // namespace petri
