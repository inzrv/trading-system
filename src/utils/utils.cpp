#include "utils.h"

#include <boost/json/src.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/system/error_code.hpp>

std::optional<boost::json::value> parse_to_json(std::string_view s)
{
    boost::json::value json_value;
    try {
        json_value = boost::json::parse(s);
    } catch (const std::exception& /*e*/) {
        return std::nullopt;
    }

    return json_value;
}

std::optional<boost::json::object> parse_to_json_object(std::string_view s)
{
    boost::system::error_code ec;
    auto value = boost::json::parse(s, ec);
    if (ec || !value.is_object()) {
        return std::nullopt;
    }

    return value.as_object();
}

std::optional<std::string> json_string(const boost::json::object& object, std::string_view field)
{
    const auto* value = object.if_contains(field);
    if (!value || !value->is_string()) {
        return std::nullopt;
    }

    return std::string(value->as_string().c_str());
}

std::optional<int64_t> json_int64(const boost::json::object& object, std::string_view field)
{
    const auto* value = object.if_contains(field);
    if (!value || !value->is_int64()) {
        return std::nullopt;
    }

    return value->as_int64();
}

std::optional<uint64_t> json_uint64(const boost::json::object& object, std::string_view field)
{
    const auto value = json_int64(object, field);
    if (!value || *value < 0) {
        return std::nullopt;
    }

    return static_cast<uint64_t>(*value);
}

const boost::json::array* json_array(const boost::json::object& object, std::string_view field) noexcept
{
    const auto* value = object.if_contains(field);
    if (!value || !value->is_array()) {
        return nullptr;
    }

    return &value->as_array();
}

const boost::json::object* json_object(const boost::json::object& object, std::string_view field) noexcept
{
    const auto* value = object.if_contains(field);
    if (!value || !value->is_object()) {
        return nullptr;
    }

    return &value->as_object();
}

std::string to_lower(std::string s)
{
    boost::algorithm::to_lower(s);
    return s;
}

int64_t now_unix_ms() noexcept
{
    using namespace std::chrono;
    return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
}
