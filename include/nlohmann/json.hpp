#pragma once

#include <cctype>
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <istream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <variant>
#include <vector>

namespace nlohmann {

class json {
public:
    using object_t = std::map<std::string, json>;
    using array_t = std::vector<json>;
    using string_t = std::string;
    using integer_t = std::int64_t;
    using float_t = double;
    using boolean_t = bool;

    json() : data_(nullptr) {}
    json(std::nullptr_t) : data_(nullptr) {}
    json(const object_t& value) : data_(value) {}
    json(object_t&& value) : data_(std::move(value)) {}
    json(const array_t& value) : data_(value) {}
    json(array_t&& value) : data_(std::move(value)) {}
    json(const string_t& value) : data_(value) {}
    json(string_t&& value) : data_(std::move(value)) {}
    json(const char* value) : data_(string_t(value)) {}
    json(float_t value) : data_(value) {}
    json(boolean_t value) : data_(value) {}

    template <typename Integer,
              typename = std::enable_if_t<std::is_integral_v<Integer> &&
                                          !std::is_same_v<std::decay_t<Integer>, boolean_t>>>
    json(Integer value) : data_(static_cast<integer_t>(value)) {}

    static json object() {
        return json(object_t{});
    }

    static json array() {
        return json(array_t{});
    }

    static json parse(const std::string& text) {
        Parser parser(text);
        json value = parser.parse_value();
        parser.skip_ws();
        if (!parser.eof()) {
            throw std::runtime_error("unexpected trailing characters in JSON");
        }
        return value;
    }

    bool is_null() const { return std::holds_alternative<std::nullptr_t>(data_); }
    bool is_object() const { return std::holds_alternative<object_t>(data_); }
    bool is_array() const { return std::holds_alternative<array_t>(data_); }
    bool is_string() const { return std::holds_alternative<string_t>(data_); }
    bool is_boolean() const { return std::holds_alternative<boolean_t>(data_); }
    bool is_number_integer() const { return std::holds_alternative<integer_t>(data_); }
    bool is_number_float() const { return std::holds_alternative<float_t>(data_); }
    bool is_number() const { return is_number_integer() || is_number_float(); }

    bool contains(const std::string& key) const {
        if (!is_object()) {
            return false;
        }
        return std::get<object_t>(data_).find(key) != std::get<object_t>(data_).end();
    }

    std::size_t size() const {
        if (is_array()) {
            return std::get<array_t>(data_).size();
        }
        if (is_object()) {
            return std::get<object_t>(data_).size();
        }
        return 0;
    }

    json& operator[](const std::string& key) {
        if (!is_object()) {
            data_ = object_t{};
        }
        return std::get<object_t>(data_)[key];
    }

    const json& operator[](const std::string& key) const {
        return at(key);
    }

    json& operator[](const char* key) {
        return (*this)[std::string(key)];
    }

    const json& operator[](const char* key) const {
        return at(std::string(key));
    }

    json& operator[](std::size_t index) {
        if (!is_array()) {
            throw std::out_of_range("JSON value is not an array");
        }
        return std::get<array_t>(data_).at(index);
    }

    const json& operator[](std::size_t index) const {
        if (!is_array()) {
            throw std::out_of_range("JSON value is not an array");
        }
        return std::get<array_t>(data_).at(index);
    }

    const json& at(const std::string& key) const {
        if (!is_object()) {
            throw std::out_of_range("JSON value is not an object");
        }
        return std::get<object_t>(data_).at(key);
    }

    json& at(const std::string& key) {
        if (!is_object()) {
            throw std::out_of_range("JSON value is not an object");
        }
        return std::get<object_t>(data_).at(key);
    }

    const json& at(std::size_t index) const {
        if (!is_array()) {
            throw std::out_of_range("JSON value is not an array");
        }
        return std::get<array_t>(data_).at(index);
    }

    json& at(std::size_t index) {
        if (!is_array()) {
            throw std::out_of_range("JSON value is not an array");
        }
        return std::get<array_t>(data_).at(index);
    }

    array_t::iterator begin() {
        return as_array().begin();
    }

    array_t::iterator end() {
        return as_array().end();
    }

    array_t::const_iterator begin() const {
        return as_array().begin();
    }

    array_t::const_iterator end() const {
        return as_array().end();
    }

    void push_back(const json& value) {
        if (!is_array()) {
            data_ = array_t{};
        }
        std::get<array_t>(data_).push_back(value);
    }

    template <typename T>
    T get() const {
        if constexpr (std::is_same_v<T, std::string>) {
            if (!is_string()) {
                throw std::runtime_error("JSON value is not a string");
            }
            return std::get<string_t>(data_);
        } else if constexpr (std::is_same_v<T, int>) {
            if (is_number_integer()) {
                return static_cast<int>(std::get<integer_t>(data_));
            }
            if (is_number_float()) {
                return static_cast<int>(std::get<float_t>(data_));
            }
            throw std::runtime_error("JSON value is not an integer");
        } else if constexpr (std::is_same_v<T, std::size_t>) {
            if (is_number_integer()) {
                return static_cast<std::size_t>(std::get<integer_t>(data_));
            }
            throw std::runtime_error("JSON value is not an unsigned integer");
        } else if constexpr (std::is_same_v<T, double>) {
            if (is_number_integer()) {
                return static_cast<double>(std::get<integer_t>(data_));
            }
            if (is_number_float()) {
                return std::get<float_t>(data_);
            }
            throw std::runtime_error("JSON value is not a number");
        } else if constexpr (std::is_same_v<T, bool>) {
            if (!is_boolean()) {
                throw std::runtime_error("JSON value is not a boolean");
            }
            return std::get<boolean_t>(data_);
        } else {
            static_assert(sizeof(T) == 0, "unsupported json::get<T>() type");
        }
    }

    template <typename T>
    T value(const std::string& key, const T& default_value) const {
        if (!contains(key)) {
            return default_value;
        }
        return at(key).get<T>();
    }

    std::string dump(int indent = -1) const {
        std::ostringstream out;
        dump_into(out, indent, 0);
        return out.str();
    }

private:
    class Parser {
    public:
        explicit Parser(std::string_view text) : text_(text) {}

        json parse_value() {
            skip_ws();
            if (eof()) {
                throw std::runtime_error("unexpected end of JSON");
            }

            const char ch = peek();
            if (ch == '{') {
                return parse_object();
            }
            if (ch == '[') {
                return parse_array();
            }
            if (ch == '"') {
                return json(parse_string());
            }
            if (ch == '-' || std::isdigit(static_cast<unsigned char>(ch)) != 0) {
                return parse_number();
            }
            if (consume_literal("true")) {
                return json(true);
            }
            if (consume_literal("false")) {
                return json(false);
            }
            if (consume_literal("null")) {
                return json(nullptr);
            }
            throw std::runtime_error("invalid JSON value");
        }

        void skip_ws() {
            while (!eof() && std::isspace(static_cast<unsigned char>(text_[position_])) != 0) {
                ++position_;
            }
        }

        bool eof() const {
            return position_ >= text_.size();
        }

    private:
        char peek() const {
            return text_[position_];
        }

        char get_char() {
            if (eof()) {
                throw std::runtime_error("unexpected end of JSON");
            }
            return text_[position_++];
        }

        bool consume_literal(std::string_view literal) {
            if (text_.substr(position_, literal.size()) == literal) {
                position_ += literal.size();
                return true;
            }
            return false;
        }

        void expect(char expected) {
            const char actual = get_char();
            if (actual != expected) {
                std::ostringstream message;
                message << "expected '" << expected << "' but found '" << actual << "'";
                throw std::runtime_error(message.str());
            }
        }

        json parse_object() {
            expect('{');
            object_t object;
            skip_ws();
            if (!eof() && peek() == '}') {
                get_char();
                return json(std::move(object));
            }

            while (true) {
                skip_ws();
                if (eof() || peek() != '"') {
                    throw std::runtime_error("expected object key");
                }
                std::string key = parse_string();
                skip_ws();
                expect(':');
                object[key] = parse_value();
                skip_ws();
                const char delimiter = get_char();
                if (delimiter == '}') {
                    break;
                }
                if (delimiter != ',') {
                    throw std::runtime_error("expected ',' or '}' in object");
                }
            }
            return json(std::move(object));
        }

        json parse_array() {
            expect('[');
            array_t array;
            skip_ws();
            if (!eof() && peek() == ']') {
                get_char();
                return json(std::move(array));
            }

            while (true) {
                array.push_back(parse_value());
                skip_ws();
                const char delimiter = get_char();
                if (delimiter == ']') {
                    break;
                }
                if (delimiter != ',') {
                    throw std::runtime_error("expected ',' or ']' in array");
                }
            }
            return json(std::move(array));
        }

        std::string parse_string() {
            expect('"');
            std::string result;
            while (!eof()) {
                const char ch = get_char();
                if (ch == '"') {
                    return result;
                }
                if (ch != '\\') {
                    result.push_back(ch);
                    continue;
                }

                const char escaped = get_char();
                switch (escaped) {
                    case '"': result.push_back('"'); break;
                    case '\\': result.push_back('\\'); break;
                    case '/': result.push_back('/'); break;
                    case 'b': result.push_back('\b'); break;
                    case 'f': result.push_back('\f'); break;
                    case 'n': result.push_back('\n'); break;
                    case 'r': result.push_back('\r'); break;
                    case 't': result.push_back('\t'); break;
                    case 'u':
                        parse_unicode_escape(result);
                        break;
                    default:
                        throw std::runtime_error("invalid JSON string escape");
                }
            }
            throw std::runtime_error("unterminated JSON string");
        }

        void parse_unicode_escape(std::string& output) {
            int codepoint = 0;
            for (int i = 0; i < 4; ++i) {
                const char ch = get_char();
                codepoint <<= 4;
                if (ch >= '0' && ch <= '9') {
                    codepoint += ch - '0';
                } else if (ch >= 'a' && ch <= 'f') {
                    codepoint += 10 + ch - 'a';
                } else if (ch >= 'A' && ch <= 'F') {
                    codepoint += 10 + ch - 'A';
                } else {
                    throw std::runtime_error("invalid unicode escape");
                }
            }

            if (codepoint <= 0x7F) {
                output.push_back(static_cast<char>(codepoint));
            } else if (codepoint <= 0x7FF) {
                output.push_back(static_cast<char>(0xC0 | (codepoint >> 6)));
                output.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
            } else {
                output.push_back(static_cast<char>(0xE0 | (codepoint >> 12)));
                output.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
                output.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
            }
        }

        json parse_number() {
            const std::size_t start = position_;
            if (peek() == '-') {
                ++position_;
            }
            if (eof()) {
                throw std::runtime_error("invalid number");
            }
            if (peek() == '0') {
                ++position_;
            } else {
                if (std::isdigit(static_cast<unsigned char>(peek())) == 0) {
                    throw std::runtime_error("invalid number");
                }
                while (!eof() && std::isdigit(static_cast<unsigned char>(peek())) != 0) {
                    ++position_;
                }
            }

            bool floating = false;
            if (!eof() && peek() == '.') {
                floating = true;
                ++position_;
                if (eof() || std::isdigit(static_cast<unsigned char>(peek())) == 0) {
                    throw std::runtime_error("invalid fractional number");
                }
                while (!eof() && std::isdigit(static_cast<unsigned char>(peek())) != 0) {
                    ++position_;
                }
            }

            if (!eof() && (peek() == 'e' || peek() == 'E')) {
                floating = true;
                ++position_;
                if (!eof() && (peek() == '+' || peek() == '-')) {
                    ++position_;
                }
                if (eof() || std::isdigit(static_cast<unsigned char>(peek())) == 0) {
                    throw std::runtime_error("invalid exponent");
                }
                while (!eof() && std::isdigit(static_cast<unsigned char>(peek())) != 0) {
                    ++position_;
                }
            }

            const std::string token(text_.substr(start, position_ - start));
            if (floating) {
                return json(std::stod(token));
            }
            return json(static_cast<integer_t>(std::stoll(token)));
        }

        std::string_view text_;
        std::size_t position_ = 0;
    };

    array_t& as_array() {
        if (!is_array()) {
            throw std::runtime_error("JSON value is not an array");
        }
        return std::get<array_t>(data_);
    }

    const array_t& as_array() const {
        if (!is_array()) {
            throw std::runtime_error("JSON value is not an array");
        }
        return std::get<array_t>(data_);
    }

    static std::string escape_string(const std::string& input) {
        std::ostringstream out;
        for (const char ch : input) {
            switch (ch) {
                case '"': out << "\\\""; break;
                case '\\': out << "\\\\"; break;
                case '\b': out << "\\b"; break;
                case '\f': out << "\\f"; break;
                case '\n': out << "\\n"; break;
                case '\r': out << "\\r"; break;
                case '\t': out << "\\t"; break;
                default:
                    if (static_cast<unsigned char>(ch) < 0x20) {
                        out << "\\u" << std::hex << std::setw(4) << std::setfill('0')
                            << static_cast<int>(static_cast<unsigned char>(ch));
                    } else {
                        out << ch;
                    }
            }
        }
        return out.str();
    }

    void write_indent(std::ostringstream& out, int indent, int depth) const {
        if (indent >= 0) {
            out << std::string(static_cast<std::size_t>(indent * depth), ' ');
        }
    }

    void dump_into(std::ostringstream& out, int indent, int depth) const {
        if (is_null()) {
            out << "null";
        } else if (is_boolean()) {
            out << (std::get<boolean_t>(data_) ? "true" : "false");
        } else if (is_number_integer()) {
            out << std::get<integer_t>(data_);
        } else if (is_number_float()) {
            out << std::setprecision(15) << std::get<float_t>(data_);
        } else if (is_string()) {
            out << '"' << escape_string(std::get<string_t>(data_)) << '"';
        } else if (is_array()) {
            const auto& array = std::get<array_t>(data_);
            out << '[';
            if (!array.empty()) {
                if (indent >= 0) {
                    out << '\n';
                }
                for (std::size_t i = 0; i < array.size(); ++i) {
                    write_indent(out, indent, depth + 1);
                    array[i].dump_into(out, indent, depth + 1);
                    if (i + 1 < array.size()) {
                        out << ',';
                    }
                    if (indent >= 0) {
                        out << '\n';
                    }
                }
                write_indent(out, indent, depth);
            }
            out << ']';
        } else {
            const auto& object = std::get<object_t>(data_);
            out << '{';
            if (!object.empty()) {
                if (indent >= 0) {
                    out << '\n';
                }
                std::size_t index = 0;
                for (const auto& [key, value] : object) {
                    write_indent(out, indent, depth + 1);
                    out << '"' << escape_string(key) << "\":";
                    if (indent >= 0) {
                        out << ' ';
                    }
                    value.dump_into(out, indent, depth + 1);
                    if (++index < object.size()) {
                        out << ',';
                    }
                    if (indent >= 0) {
                        out << '\n';
                    }
                }
                write_indent(out, indent, depth);
            }
            out << '}';
        }
    }

    std::variant<std::nullptr_t, object_t, array_t, string_t, integer_t, float_t, boolean_t> data_;
};

inline std::istream& operator>>(std::istream& input, json& value) {
    std::ostringstream buffer;
    buffer << input.rdbuf();
    value = json::parse(buffer.str());
    return input;
}

} // namespace nlohmann
