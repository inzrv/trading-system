#pragma once

#include <boost/json.hpp>

#include <optional>
#include <string_view>
#include <chrono>

std::optional<boost::json::value> parse_to_json(std::string_view s);

std::string to_lower(std::string s);

int64_t now_unix_ms() noexcept;
