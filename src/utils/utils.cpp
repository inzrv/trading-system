#include "utils.h"

#include <boost/json/src.hpp>
#include <boost/algorithm/string.hpp>

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

std::string to_lower(std::string s)
{
    boost::algorithm::to_lower(s);
    return s;
}
