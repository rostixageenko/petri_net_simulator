#pragma once

#include <map>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>

namespace petri {

struct Error {
    std::string code;
    std::string message;
    std::map<std::string, std::string> details;
};

template <typename T>
class Result {
public:
    static Result success(T value) {
        Result result;
        result.value_ = std::move(value);
        return result;
    }

    static Result failure(Error error) {
        Result result;
        result.error_ = std::move(error);
        return result;
    }

    bool has_value() const {
        return value_.has_value();
    }

    explicit operator bool() const {
        return has_value();
    }

    T& value() {
        if (!value_) {
            throw std::logic_error("attempted to access failed Result value");
        }
        return *value_;
    }

    const T& value() const {
        if (!value_) {
            throw std::logic_error("attempted to access failed Result value");
        }
        return *value_;
    }

    const Error& error() const {
        if (value_) {
            throw std::logic_error("attempted to access successful Result error");
        }
        return error_;
    }

private:
    std::optional<T> value_;
    Error error_;
};

template <>
class Result<void> {
public:
    static Result success() {
        return Result(true, Error{});
    }

    static Result failure(Error error) {
        return Result(false, std::move(error));
    }

    bool has_value() const {
        return ok_;
    }

    explicit operator bool() const {
        return ok_;
    }

    const Error& error() const {
        if (ok_) {
            throw std::logic_error("attempted to access successful Result error");
        }
        return error_;
    }

private:
    Result(bool ok, Error error) : ok_(ok), error_(std::move(error)) {}

    bool ok_ = false;
    Error error_;
};

inline Error make_error(std::string code, std::string message,
                        std::map<std::string, std::string> details = {}) {
    return Error{std::move(code), std::move(message), std::move(details)};
}

} // namespace petri
