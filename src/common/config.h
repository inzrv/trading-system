#pragma once

#include "market.h"
#include "utils/utils.h"

#include <boost/json.hpp>

#include <optional>
#include <string_view>

struct IConfig
{
    virtual ~IConfig() = default;
    virtual Market market() const noexcept = 0;

    bool from_string(std::string_view s)
    {
        const auto parse_to_json_res = parse_to_json(s);
        if (!parse_to_json_res) {
            return false;
        }

        boost::json::value json_value = *parse_to_json_res;
        if (!json_value.is_object()) {
            return false; 
        }

        return from_json(json_value.as_object());
    }

protected:
    virtual bool from_json(const boost::json::object& json) = 0;
};

std::optional<Market> detect_market_from_config(std::string_view payload);
