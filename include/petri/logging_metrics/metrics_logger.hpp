#pragma once

#include "petri/common/result.hpp"

#include <nlohmann/json.hpp>

#include <cstddef>
#include <string>
#include <vector>

namespace petri {

struct RunMetricsInput {
    std::string request_id;
    std::string algorithm_name;
    std::string task_type;
    std::size_t place_count = 0;
    std::size_t transition_count = 0;
    std::size_t arc_count = 0;
    std::size_t built_state_count = 0;
    std::size_t scanned_edge_count = 0;
    double elapsed_ms = 0.0;
    bool found = false;
    double path_cost = 0.0;
    std::size_t path_length = 0;
    std::string error_message;
};

struct RunMetricsRecord {
    std::string request_id;
    std::string algorithm_name;
    std::string task_type;
    std::size_t place_count = 0;
    std::size_t transition_count = 0;
    std::size_t arc_count = 0;
    std::size_t built_state_count = 0;
    std::size_t scanned_edge_count = 0;
    double elapsed_ms = 0.0;
    bool found = false;
    double path_cost = 0.0;
    std::size_t path_length = 0;
    std::string error_message;
};

RunMetricsRecord create_metrics_record(RunMetricsInput input);

nlohmann::json metrics_record_to_json(const RunMetricsRecord& record);
Result<RunMetricsRecord> metrics_record_from_json(const nlohmann::json& data);

std::string default_metrics_log_path();

Result<void> append_metrics_record(const RunMetricsRecord& record);
Result<void> append_metrics_record(const RunMetricsRecord& record, const std::string& path);

Result<std::vector<RunMetricsRecord>> read_metrics_records();
Result<std::vector<RunMetricsRecord>> read_metrics_records(const std::string& path);

} // namespace petri
