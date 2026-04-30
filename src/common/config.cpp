#include "config.h"

std::optional<Market> detect_market_from_config(std::string_view payload)
{
    const auto json_res = parse_to_json_object(payload);
    if (!json_res) {
        return std::nullopt;
    }

    const auto market_json = json_string(*json_res, "market");
    if (!market_json) {
        return std::nullopt;
    }

    const auto market = market_from_string(*market_json);
    if (market == Market::UNKNOWN) {
        return std::nullopt;
    }

    return market;
}
