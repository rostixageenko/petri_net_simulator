#include "petri/storage_api/storage_api.hpp"

#include <doctest/doctest.h>

#include <filesystem>
#include <fstream>
#include <string>

using namespace petri;

namespace {

StorageConfig test_config() {
    StorageConfig config;
    config.root = "data/test_storage_api";
    std::filesystem::remove_all(config.root);
    return config;
}

nlohmann::json sample_model() {
    return nlohmann::json::parse(R"({
        "name": "unit_model",
        "places": [{"id": "p1", "label": "P1", "tokens": 1}],
        "transitions": [{"id": "t1", "label": "T1", "fire_time": 1.0}],
        "arcs": [{"id": "a1", "source": "p1", "target": "t1", "weight": 1}]
    })");
}

} // namespace

TEST_CASE("storage api saves and loads Petri net models") {
    auto config = test_config();

    auto saved = save_model("unit_model", sample_model(), config);
    REQUIRE(saved.has_value());
    CHECK_EQ(saved.value().id, "unit_model");
    CHECK(std::filesystem::exists(saved.value().path));
    CHECK_EQ(saved.value().path.parent_path(), models_directory(config));

    auto loaded = load_model("unit_model", config);
    REQUIRE(loaded.has_value());
    CHECK_EQ(loaded.value().at("name").get<std::string>(), "unit_model");
    CHECK_EQ(loaded.value().at("places").size(), 1);

    std::filesystem::remove_all(config.root);
}

TEST_CASE("storage api saves run results with unique run id and loads them") {
    auto config = test_config();
    nlohmann::json result = nlohmann::json::object();
    result["status"] = "ok";
    result["algorithm"] = "bfs";

    auto first = save_run_result(result, config);
    auto second = save_run_result(result, config);
    REQUIRE(first.has_value());
    REQUIRE(second.has_value());
    CHECK(!first.value().id.empty());
    CHECK(!second.value().id.empty());
    CHECK(first.value().id != second.value().id);
    CHECK_EQ(first.value().path.parent_path(), runs_directory(config));

    auto loaded = load_run_result(first.value().id, config);
    REQUIRE(loaded.has_value());
    CHECK_EQ(loaded.value().at("run_id").get<std::string>(), first.value().id);
    CHECK_EQ(loaded.value().at("status").get<std::string>(), "ok");
    CHECK_EQ(loaded.value().at("algorithm").get<std::string>(), "bfs");

    std::filesystem::remove_all(config.root);
}

TEST_CASE("storage api saves metrics as JSONL") {
    auto config = test_config();
    nlohmann::json metrics = nlohmann::json::object();
    metrics["algorithm"] = "dijkstra";
    metrics["found"] = true;
    metrics["path_cost"] = 2.5;

    auto saved = save_metrics("run-42", metrics, config);
    REQUIRE(saved.has_value());
    CHECK_EQ(saved.value().id, "run-42");
    CHECK_EQ(saved.value().path.parent_path(), metrics_directory(config));
    CHECK(std::filesystem::exists(saved.value().path));

    {
        std::ifstream input(saved.value().path);
        REQUIRE(static_cast<bool>(input));
        std::string line;
        std::getline(input, line);
        REQUIRE(!line.empty());
        const auto parsed = nlohmann::json::parse(line);
        CHECK_EQ(parsed.at("run_id").get<std::string>(), "run-42");
        CHECK_EQ(parsed.at("algorithm").get<std::string>(), "dijkstra");
        CHECK(parsed.at("found").get<bool>());
    }

    std::filesystem::remove_all(config.root);
}

TEST_CASE("storage api saves array metrics as multiple JSONL lines") {
    auto config = test_config();
    nlohmann::json metrics = nlohmann::json::array();
    nlohmann::json first = nlohmann::json::object();
    first["algorithm"] = "bfs";
    first["path_length"] = 1;
    nlohmann::json second = nlohmann::json::object();
    second["algorithm"] = "dfs";
    second["path_length"] = 2;
    metrics.push_back(first);
    metrics.push_back(second);

    auto saved = save_metrics("run-array", metrics, config);
    REQUIRE(saved.has_value());

    {
        std::ifstream input(saved.value().path);
        REQUIRE(static_cast<bool>(input));
        std::string first_line;
        std::string second_line;
        std::string third_line;
        std::getline(input, first_line);
        std::getline(input, second_line);
        std::getline(input, third_line);
        CHECK(!first_line.empty());
        CHECK(!second_line.empty());
        CHECK(third_line.empty());
        CHECK_EQ(nlohmann::json::parse(first_line).at("run_id").get<std::string>(), "run-array");
        CHECK_EQ(nlohmann::json::parse(second_line).at("algorithm").get<std::string>(), "dfs");
    }

    std::filesystem::remove_all(config.root);
}

TEST_CASE("storage api rejects empty model ids") {
    auto config = test_config();

    auto saved = save_model("", sample_model(), config);
    REQUIRE(!saved.has_value());
    CHECK_EQ(saved.error().code, "INVALID_STORAGE_ID");

    std::filesystem::remove_all(config.root);
}
