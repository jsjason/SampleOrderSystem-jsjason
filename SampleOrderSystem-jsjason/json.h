#pragma once
// Minimal JSON library for SampleOrderSystem
// nlohmann::json compatible API (subset)

#include <string>
#include <vector>
#include <map>
#include <variant>
#include <optional>
#include <stdexcept>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <cstdio>

namespace nlohmann {

class json {
public:
    using object_t = std::map<std::string, json>;
    using array_t  = std::vector<json>;

    using value_t = std::variant<
        std::nullptr_t,
        bool,
        std::int64_t,
        double,
        std::string,
        array_t,
        object_t
    >;

    // ---- Constructors ----

    json()                       : val_(nullptr) {}
    json(std::nullptr_t)         : val_(nullptr) {}
    json(bool v)                 : val_(v) {}
    json(int v)                  : val_(static_cast<std::int64_t>(v)) {}
    json(std::int64_t v)         : val_(v) {}
    json(double v)               : val_(v) {}
    json(const char* v)          : val_(std::string(v)) {}
    json(const std::string& v)   : val_(v) {}
    json(std::string&& v)        : val_(std::move(v)) {}
    json(array_t v)              : val_(std::move(v)) {}
    json(object_t v)             : val_(std::move(v)) {}

    // ---- Type checks ----

    bool is_null()   const { return std::holds_alternative<std::nullptr_t>(val_); }
    bool is_bool()   const { return std::holds_alternative<bool>(val_); }
    bool is_int()    const { return std::holds_alternative<std::int64_t>(val_); }
    bool is_double() const { return std::holds_alternative<double>(val_); }
    bool is_number() const { return is_int() || is_double(); }
    bool is_string() const { return std::holds_alternative<std::string>(val_); }
    bool is_array()  const { return std::holds_alternative<array_t>(val_); }
    bool is_object() const { return std::holds_alternative<object_t>(val_); }

    // ---- get<T> ----

    template<typename T> T get() const;

    // ---- Object access ----

    json& operator[](const std::string& key) {
        if (is_null()) val_ = object_t{};
        return std::get<object_t>(val_)[key];
    }
    const json& operator[](const std::string& key) const {
        return std::get<object_t>(val_).at(key);
    }
    const json& at(const std::string& key) const {
        return std::get<object_t>(val_).at(key);
    }
    bool contains(const std::string& key) const {
        if (!is_object()) return false;
        return std::get<object_t>(val_).count(key) > 0;
    }

    // ---- Array access ----

    json& operator[](std::size_t idx) {
        return std::get<array_t>(val_)[idx];
    }
    const json& operator[](std::size_t idx) const {
        return std::get<array_t>(val_)[idx];
    }
    void push_back(const json& v) {
        if (is_null()) val_ = array_t{};
        std::get<array_t>(val_).push_back(v);
    }

    // ---- Shared ----

    std::size_t size() const {
        if (is_array())  return std::get<array_t>(val_).size();
        if (is_object()) return std::get<object_t>(val_).size();
        return 0;
    }
    bool empty() const { return size() == 0; }

    // ---- Iteration (array only) ----

    array_t::const_iterator begin() const { return std::get<array_t>(val_).begin(); }
    array_t::const_iterator end()   const { return std::get<array_t>(val_).end(); }
    array_t::iterator       begin()       { return std::get<array_t>(val_).begin(); }
    array_t::iterator       end()         { return std::get<array_t>(val_).end(); }

    // ---- Factory ----

    static json array()  { json j; j.val_ = array_t{};  return j; }
    static json object() { json j; j.val_ = object_t{}; return j; }

    // ---- Serialization ----

    std::string dump(int indent = -1) const {
        std::ostringstream oss;
        dumpTo(oss, indent, 0);
        return oss.str();
    }

    // ---- Parsing ----

    static json parse(const std::string& s) {
        std::size_t pos = 0;
        return parseValue(s, pos);
    }
    static json parse(std::istream& is) {
        std::string content(
            (std::istreambuf_iterator<char>(is)),
             std::istreambuf_iterator<char>());
        return parse(content);
    }

private:
    value_t val_;

    // ---- Serialization helpers ----

    static std::string doubleToStr(double v) {
        char buf[64];
        std::snprintf(buf, sizeof(buf), "%.10g", v);
        return buf;
    }
    static std::string escapeStr(const std::string& s) {
        std::string r;
        r.reserve(s.size());
        for (unsigned char c : s) {
            switch (c) {
                case '"':  r += "\\\""; break;
                case '\\': r += "\\\\"; break;
                case '\n': r += "\\n";  break;
                case '\r': r += "\\r";  break;
                case '\t': r += "\\t";  break;
                default:   r += static_cast<char>(c); break;
            }
        }
        return r;
    }

    void dumpTo(std::ostringstream& oss, int indent, int depth) const {
        const bool pretty  = indent > 0;
        const std::string nl     = pretty ? "\n"  : "";
        const std::string sp     = pretty ? " "   : "";
        const std::string pad    = pretty ? std::string(static_cast<std::size_t>((depth + 1) * indent), ' ') : "";
        const std::string padEnd = pretty ? std::string(static_cast<std::size_t>(depth * indent), ' ') : "";

        if (is_null())   { oss << "null"; return; }
        if (is_bool())   { oss << (std::get<bool>(val_) ? "true" : "false"); return; }
        if (is_int())    { oss << std::get<std::int64_t>(val_); return; }
        if (is_double()) { oss << doubleToStr(std::get<double>(val_)); return; }
        if (is_string()) { oss << '"' << escapeStr(std::get<std::string>(val_)) << '"'; return; }

        if (is_array()) {
            const auto& arr = std::get<array_t>(val_);
            if (arr.empty()) { oss << "[]"; return; }
            oss << '[' << nl;
            for (std::size_t i = 0; i < arr.size(); ++i) {
                oss << pad;
                arr[i].dumpTo(oss, indent, depth + 1);
                if (i + 1 < arr.size()) oss << ',';
                oss << nl;
            }
            oss << padEnd << ']';
            return;
        }

        // object
        const auto& obj = std::get<object_t>(val_);
        if (obj.empty()) { oss << "{}"; return; }
        oss << '{' << nl;
        std::size_t i = 0;
        for (const auto& [k, v] : obj) {
            oss << pad << '"' << escapeStr(k) << '"' << ':' << sp;
            v.dumpTo(oss, indent, depth + 1);
            if (++i < obj.size()) oss << ',';
            oss << nl;
        }
        oss << padEnd << '}';
    }

    // ---- Parsing helpers ----

    static void skipWS(const std::string& s, std::size_t& pos) {
        while (pos < s.size() && static_cast<unsigned char>(s[pos]) <= ' ') ++pos;
    }

    static json parseValue(const std::string& s, std::size_t& pos) {
        skipWS(s, pos);
        if (pos >= s.size()) throw std::runtime_error("Unexpected end of input");
        char c = s[pos];
        if (c == '{') return parseObject(s, pos);
        if (c == '[') return parseArray(s, pos);
        if (c == '"') return json(parseString(s, pos));
        if (c == 't') { pos += 4; return json(true); }
        if (c == 'f') { pos += 5; return json(false); }
        if (c == 'n') { pos += 4; return json(nullptr); }
        if (c == '-' || (c >= '0' && c <= '9')) return parseNumber(s, pos);
        throw std::runtime_error(std::string("Unexpected char: ") + c);
    }

    static json parseObject(const std::string& s, std::size_t& pos) {
        ++pos; // '{'
        json result = json::object();
        skipWS(s, pos);
        if (pos < s.size() && s[pos] == '}') { ++pos; return result; }
        while (true) {
            skipWS(s, pos);
            std::string key = parseString(s, pos);
            skipWS(s, pos);
            if (s[pos] != ':') throw std::runtime_error("Expected ':'");
            ++pos;
            result[key] = parseValue(s, pos);
            skipWS(s, pos);
            if (s[pos] == '}') { ++pos; break; }
            if (s[pos] != ',') throw std::runtime_error("Expected ',' or '}'");
            ++pos;
        }
        return result;
    }

    static json parseArray(const std::string& s, std::size_t& pos) {
        ++pos; // '['
        json result = json::array();
        skipWS(s, pos);
        if (pos < s.size() && s[pos] == ']') { ++pos; return result; }
        while (true) {
            result.push_back(parseValue(s, pos));
            skipWS(s, pos);
            if (s[pos] == ']') { ++pos; break; }
            if (s[pos] != ',') throw std::runtime_error("Expected ',' or ']'");
            ++pos;
        }
        return result;
    }

    static std::string parseString(const std::string& s, std::size_t& pos) {
        if (s[pos] != '"') throw std::runtime_error("Expected '\"'");
        ++pos;
        std::string result;
        while (pos < s.size() && s[pos] != '"') {
            if (s[pos] == '\\' && pos + 1 < s.size()) {
                ++pos;
                switch (s[pos]) {
                    case '"':  result += '"';  break;
                    case '\\': result += '\\'; break;
                    case '/':  result += '/';  break;
                    case 'n':  result += '\n'; break;
                    case 'r':  result += '\r'; break;
                    case 't':  result += '\t'; break;
                    default:   result += s[pos]; break;
                }
            } else {
                result += s[pos];
            }
            ++pos;
        }
        if (pos >= s.size()) throw std::runtime_error("Unterminated string");
        ++pos; // closing '"'
        return result;
    }

    static json parseNumber(const std::string& s, std::size_t& pos) {
        std::size_t start = pos;
        bool isFloat = false;
        if (s[pos] == '-') ++pos;
        while (pos < s.size() && s[pos] >= '0' && s[pos] <= '9') ++pos;
        if (pos < s.size() && s[pos] == '.') { isFloat = true; ++pos; }
        while (pos < s.size() && s[pos] >= '0' && s[pos] <= '9') ++pos;
        if (pos < s.size() && (s[pos] == 'e' || s[pos] == 'E')) {
            isFloat = true; ++pos;
            if (pos < s.size() && (s[pos] == '+' || s[pos] == '-')) ++pos;
            while (pos < s.size() && s[pos] >= '0' && s[pos] <= '9') ++pos;
        }
        const std::string num = s.substr(start, pos - start);
        if (isFloat) return json(std::stod(num));
        return json(static_cast<std::int64_t>(std::stoll(num)));
    }
};

// ---- get<T> specializations ----

template<> inline bool        json::get<bool>()        const { return std::get<bool>(val_); }
template<> inline int         json::get<int>()         const {
    if (is_int())    return static_cast<int>(std::get<std::int64_t>(val_));
    if (is_double()) return static_cast<int>(std::get<double>(val_));
    throw std::bad_variant_access{};
}
template<> inline std::int64_t json::get<std::int64_t>() const { return std::get<std::int64_t>(val_); }
template<> inline double       json::get<double>()       const {
    if (is_double()) return std::get<double>(val_);
    if (is_int())    return static_cast<double>(std::get<std::int64_t>(val_));
    throw std::bad_variant_access{};
}
template<> inline std::string  json::get<std::string>()  const { return std::get<std::string>(val_); }

} // namespace nlohmann
