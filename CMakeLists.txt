cmake_minimum_required(VERSION 3.20)

project(petri_net_simulator VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(petri_core
    src/algorithms/algorithms.cpp
    src/algorithm_selector/algorithm_selector.cpp
    src/core_pn/interpreter.cpp
    src/core_pn/petri_net.cpp
    src/graph_models/directed_graph.cpp
    src/graph_models/reachability.cpp
    src/graph_models/star_forms.cpp
    src/logging_metrics/metrics_logger.cpp
    src/runtime/algorithm_runtime.cpp
    src/service_adapter/json_api.cpp
    src/storage_api/storage_api.cpp
)

target_include_directories(petri_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

if(MSVC)
    target_compile_options(petri_core PRIVATE /W4 /permissive- /FS)
else()
    target_compile_options(petri_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_executable(petri_cli src/cli/petri_cli.cpp)
target_link_libraries(petri_cli PRIVATE petri_core)

enable_testing()

add_executable(petri_tests
    tests/test_main.cpp
    tests/algorithm_selector_tests.cpp
    tests/algorithms_tests.cpp
    tests/core_pn_tests.cpp
    tests/graph_models_tests.cpp
    tests/integration_tests.cpp
    tests/metrics_logger_tests.cpp
    tests/runtime_tests.cpp
    tests/service_adapter_tests.cpp
    tests/storage_api_tests.cpp
)
target_link_libraries(petri_tests PRIVATE petri_core)
if(MSVC)
    target_compile_options(petri_tests PRIVATE /W4 /permissive- /FS)
endif()

add_test(NAME petri_tests COMMAND petri_tests)
set_tests_properties(petri_tests PROPERTIES WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
