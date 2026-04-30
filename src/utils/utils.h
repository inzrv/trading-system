#pragma once

#include <boost/json.hpp>

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <chrono>

std::optional<boost::json::value> parse_to_json(std::string_view s);
std::optional<boost::json::object> parse_to_json_object(std::string_view s);

std::optional<std::string> json_string(const boost::json::object& object, std::string_view field);
std::optional<int64_t> json_int64(const boost::json::object& object, std::string_view field);
std::optional<uint64_t> json_uint64(const boost::json::object& object, std::string_view field);
const boost::json::array* json_array(const boost::json::object& object, std::string_view field) noexcept;
const boost::json::object* json_object(const boost::json::object& object, std::string_view field) noexcept;

std::string to_lower(std::string s);

int64_t now_unix_ms() noexcept;
