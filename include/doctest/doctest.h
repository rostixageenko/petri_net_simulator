#pragma once

#include <exception>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace doctest {

using TestFunction = void (*)();

struct TestCase {
    std::string name;
    TestFunction function;
};

inline std::vector<TestCase>& registry() {
    static std::vector<TestCase> tests;
    return tests;
}

struct Registrar {
    Registrar(const char* name, TestFunction function) {
        registry().push_back(TestCase{name, function});
    }
};

class TestFailure : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

inline int& current_failures() {
    static int failures = 0;
    return failures;
}

inline void check(bool condition, const char* expression, const char* file, int line, bool fatal) {
    if (condition) {
        return;
    }

    ++current_failures();
    std::cerr << file << ":" << line << ": check failed: " << expression << '\n';
    if (fatal) {
        throw TestFailure(expression);
    }
}

inline int run_tests() {
    int failed_cases = 0;
    for (const auto& test : registry()) {
        current_failures() = 0;
        try {
            test.function();
        } catch (const TestFailure&) {
            // The failing assertion has already been reported.
        } catch (const std::exception& ex) {
            ++current_failures();
            std::cerr << "Unhandled exception in test '" << test.name << "': " << ex.what() << '\n';
        } catch (...) {
            ++current_failures();
            std::cerr << "Unhandled non-standard exception in test '" << test.name << "'\n";
        }

        if (current_failures() != 0) {
            ++failed_cases;
            std::cerr << "[failed] " << test.name << " (" << current_failures() << " checks)\n";
        } else {
            std::cout << "[passed] " << test.name << '\n';
        }
    }

    std::cout << registry().size() << " test case(s), " << failed_cases << " failure(s)\n";
    return failed_cases == 0 ? 0 : 1;
}

} // namespace doctest

#define DOCTEST_DETAIL_CONCAT_IMPL(lhs, rhs) lhs##rhs
#define DOCTEST_DETAIL_CONCAT(lhs, rhs) DOCTEST_DETAIL_CONCAT_IMPL(lhs, rhs)

#define TEST_CASE(name)                                                                           \
    static void DOCTEST_DETAIL_CONCAT(doctest_test_, __LINE__)();                                  \
    static ::doctest::Registrar DOCTEST_DETAIL_CONCAT(doctest_registrar_, __LINE__)(               \
        name, &DOCTEST_DETAIL_CONCAT(doctest_test_, __LINE__));                                    \
    static void DOCTEST_DETAIL_CONCAT(doctest_test_, __LINE__)()

#define CHECK(expression) ::doctest::check(static_cast<bool>(expression), #expression, __FILE__, __LINE__, false)
#define REQUIRE(expression) ::doctest::check(static_cast<bool>(expression), #expression, __FILE__, __LINE__, true)
#define CHECK_EQ(lhs, rhs) ::doctest::check(((lhs) == (rhs)), #lhs " == " #rhs, __FILE__, __LINE__, false)
#define REQUIRE_EQ(lhs, rhs) ::doctest::check(((lhs) == (rhs)), #lhs " == " #rhs, __FILE__, __LINE__, true)

#ifdef DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
int main() {
    return ::doctest::run_tests();
}
#endif
