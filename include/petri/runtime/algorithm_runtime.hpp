#pragma once

#include "petri/algorithms/algorithms.hpp"

#include <string>

namespace petri {

Result<AlgorithmResult> run_algorithm(const std::string& name, const DirectedGraph& graph,
                                      const AlgorithmParams& params);

} // namespace petri
