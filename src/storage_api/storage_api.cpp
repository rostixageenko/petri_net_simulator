#include "petri/storage_api/storage_api.hpp"

#include <chrono>
#include <fstream>
#include <random>
#include <sstream>
#include <utility>

namespace petri {
namespace {

std::string sanitize_storage_id(const std::string& id) {
    std::string sanitized;
    sanitized.reserve(id.size());
    for (const char ch : id) {
        const bool is_digit = ch >= '0' && ch <= '9';
        const bool is_lower = ch >= 'a' && ch <= 'z';
        const bool is_upper = ch >= 'A' && ch <= 'Z';
        if (is_digit || is_lower || is_upper || ch == '_' || ch == '-' || ch == '.') {
            sanitized.push_back(ch);
        } else {
            sanitized.push_back('_');
        }
    }
    while (!sanitized.empty() && sanitized.front() == '.') {
        sanitized.erase(sanitized.begin());
    }
    while (!sanitized.empty() && sanitized.back() == '.') {
        sanitized.pop_back();
    }
    return sanitized;
}

Result<std::string> normalize_storage_id(const std::string& id) {
    const std::string sanitized = sanitize_storage_id(id);
    if (sanitized.empty()) {
        return Result<std::string>::failure(make_error("INVALID_STORAGE_ID", "Storage id must not be empty"));
    }
    return Result<std::string>::success(sanitized);
}

Result<void> ensure_directory(const std::filesystem::path& directory) {
    try {
        std::filesystem::create_directories(directory);
        return Result<void>::success();
    } catch (const std::exception& ex) {
        return Result<void>::failure(make_error("STORAGE_ERROR", ex.what(), {{"path", directory.string()}}));
    }
}

Result<StoredArtifact> write_json_file(const std::filesystem::path& directory, const std::string& id,
                                       const nlohmann::json& data) {
    auto normalized_id = normalize_storage_id(id);
    if (!normalized_id) {
        return Result<StoredArtifact>::failure(normalized_id.error());
    }
    auto directory_result = ensure_directory(directory);
    if (!directory_result) {
        return Result<StoredArtifact>::failure(directory_result.error());
    }

    const auto path = directory / (normalized_id.value() + ".json");
    std::ofstream output(path);
    if (!output) {
        return Result<StoredArtifact>::failure(
            make_error("STORAGE_ERROR", "Could not open file for writing", {{"path", path.string()}}));
    }
    output << data.dump(2);
    if (!output) {
        return Result<StoredArtifact>::failure(
            make_error("STORAGE_ERROR", "Could not write JSON file", {{"path", path.string()}}));
    }

    return Result<StoredArtifact>::success(StoredArtifact{normalized_id.value(), path});
}

Result<nlohmann::json> read_json_file(const std::filesystem::path& directory, const std::string& id) {
    auto normalized_id = normalize_storage_id(id);
    if (!normalized_id) {
        return Result<nlohmann::json>::failure(normalized_id.error());
    }

    const auto path = directory / (normalized_id.value() + ".json");
    std::ifstream input(path);
    if (!input) {
        return Result<nlohmann::json>::failure(
            make_error("STORAGE_ERROR", "Could not open file for reading", {{"path", path.string()}}));
    }

    try {
        std::ostringstream buffer;
        buffer << input.rdbuf();
        return Result<nlohmann::json>::success(nlohmann::json::parse(buffer.str()));
    } catch (const std::exception& ex) {
        return Result<nlohmann::json>::failure(
            make_error("INVALID_JSON", ex.what(), {{"path", path.string()}}));
    }
}

nlohmann::json with_run_id(const nlohmann::json& data, const std::string& run_id, const std::string& wrapper_field) {
    if (data.is_object()) {
        nlohmann::json result = data;
        result["run_id"] = run_id;
        return result;
    }

    nlohmann::json result = nlohmann::json::object();
    result["run_id"] = run_id;
    result[wrapper_field] = data;
    return result;
}

Result<void> append_metrics_line(std::ofstream& output, const nlohmann::json& item,
                                 const std::string& run_id) {
    const auto line = with_run_id(item, run_id, "metrics");
    output << line.dump() << '\n';
    if (!output) {
        return Result<void>::failure(make_error("STORAGE_ERROR", "Could not write metrics JSONL line"));
    }
    return Result<void>::success();
}

} // namespace

std::filesystem::path models_directory(const StorageConfig& config) {
    return config.root / "models";
}

std::filesystem::path runs_directory(const StorageConfig& config) {
    return config.root / "runs";
}

std::filesystem::path metrics_directory(const StorageConfig& config) {
    return config.root / "metrics";
}

std::string generate_run_id() {
    const auto now = std::chrono::system_clock::now().time_since_epoch().count();
    std::random_device random_device;
    std::ostringstream stream;
    stream << "run_" << now << '_' << random_device();
    return stream.str();
}

Result<StoredArtifact> save_model(const std::string& model_id, const nlohmann::json& model,
                                  const StorageConfig& config) {
    if (!model.is_object()) {
        return Result<StoredArtifact>::failure(
            make_error("INVALID_JSON", "Petri net model must be a JSON object"));
    }
    return write_json_file(models_directory(config), model_id, model);
}

Result<StoredArtifact> save_model(const nlohmann::json& model, const StorageConfig& config) {
    if (model.is_object() && model.contains("name") && model.at("name").is_string()) {
        return save_model(model.at("name").get<std::string>(), model, config);
    }
    return save_model("model_" + generate_run_id(), model, config);
}

Result<nlohmann::json> load_model(const std::string& model_id, const StorageConfig& config) {
    return read_json_file(models_directory(config), model_id);
}

Result<StoredArtifact> save_run_result(const nlohmann::json& run_result, const StorageConfig& config) {
    const std::string run_id = generate_run_id();
    return write_json_file(runs_directory(config), run_id, with_run_id(run_result, run_id, "result"));
}

Result<nlohmann::json> load_run_result(const std::string& run_id, const StorageConfig& config) {
    return read_json_file(runs_directory(config), run_id);
}

Result<StoredArtifact> save_metrics(const std::string& run_id, const nlohmann::json& metrics,
                                    const StorageConfig& config) {
    auto normalized_id = normalize_storage_id(run_id);
    if (!normalized_id) {
        return Result<StoredArtifact>::failure(normalized_id.error());
    }
    if (!metrics.is_object() && !metrics.is_array()) {
        return Result<StoredArtifact>::failure(
            make_error("INVALID_JSON", "Metrics must be a JSON object or an array of JSON objects"));
    }

    const auto directory = metrics_directory(config);
    auto directory_result = ensure_directory(directory);
    if (!directory_result) {
        return Result<StoredArtifact>::failure(directory_result.error());
    }

    const auto path = directory / (normalized_id.value() + ".jsonl");
    std::ofstream output(path, std::ios::app);
    if (!output) {
        return Result<StoredArtifact>::failure(
            make_error("STORAGE_ERROR", "Could not open metrics file for writing", {{"path", path.string()}}));
    }

    if (metrics.is_array()) {
        for (const auto& item : metrics) {
            auto line_result = append_metrics_line(output, item, normalized_id.value());
            if (!line_result) {
                auto error = line_result.error();
                error.details["path"] = path.string();
                return Result<StoredArtifact>::failure(std::move(error));
            }
        }
    } else {
        auto line_result = append_metrics_line(output, metrics, normalized_id.value());
        if (!line_result) {
            auto error = line_result.error();
            error.details["path"] = path.string();
            return Result<StoredArtifact>::failure(std::move(error));
        }
    }

    return Result<StoredArtifact>::success(StoredArtifact{normalized_id.value(), path});
}

} // namespace petri
