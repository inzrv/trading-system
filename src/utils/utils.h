#pragma once

#include <boost/json.hpp>

#include <optional>
#include <string_view>

std::optional<boost::json::value> parse_to_json(std::string_view s);
