#pragma once

#include "petri/common/result.hpp"

#include <nlohmann/json.hpp>

#include <filesystem>
#include <string>

namespace petri {

struct StorageConfig {
    std::filesystem::path root = "data";
};

struct StoredArtifact {
    std::string id;
    std::filesystem::path path;
};

std::filesystem::path models_directory(const StorageConfig& config = {});
std::filesystem::path runs_directory(const StorageConfig& config = {});
std::filesystem::path metrics_directory(const StorageConfig& config = {});

std::string generate_run_id();

Result<StoredArtifact> save_model(const std::string& model_id, const nlohmann::json& model,
                                  const StorageConfig& config = {});
Result<StoredArtifact> save_model(const nlohmann::json& model, const StorageConfig& config = {});
Result<nlohmann::json> load_model(const std::string& model_id, const StorageConfig& config = {});

Result<StoredArtifact> save_run_result(const nlohmann::json& run_result, const StorageConfig& config = {});
Result<nlohmann::json> load_run_result(const std::string& run_id, const StorageConfig& config = {});

Result<StoredArtifact> save_metrics(const std::string& run_id, const nlohmann::json& metrics,
                                    const StorageConfig& config = {});

} // namespace petri
