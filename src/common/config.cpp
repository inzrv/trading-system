#include "config.h"

std::optional<Market> detect_market_from_config(std::string_view payload)
{
    const auto json_res = parse_to_json(payload);
    if (!json_res || !json_res->is_object()) {
        return std::nullopt;
    }

    const auto& json = json_res->as_object();
    const auto market_json = json.if_contains("market");
    if (!market_json || !market_json->is_string()) {
        return std::nullopt;
    }

    const auto market = market_from_string(market_json->as_string().c_str());
    if (market == Market::UNKNOWN) {
        return std::nullopt;
    }

    return market;
}
