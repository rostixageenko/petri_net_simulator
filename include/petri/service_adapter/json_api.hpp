#pragma once

#include "petri/common/result.hpp"
#include "petri/core_pn/interpreter.hpp"
#include "petri/graph_models/reachability.hpp"
#include "petri/runtime/algorithm_runtime.hpp"

#include <nlohmann/json.hpp>

#include <string>

namespace petri {

class JsonServiceAdapter {
public:
    nlohmann::json handle(const nlohmann::json& request) const;
    std::string handle_json(const std::string& request_json) const;
};

nlohmann::json error_response(const std::string& request_id, const Error& error);
nlohmann::json simulate_request(const nlohmann::json& request);
nlohmann::json algorithm_request(const nlohmann::json& request);
nlohmann::json handle_request(const nlohmann::json& request);

std::string handle_request_json(const std::string& request_json);

} // namespace petri
