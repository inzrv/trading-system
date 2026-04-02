#include "config.h"

namespace binance
{

bool DepthStreamConfig::from_json(const boost::json::object& json)
{
    const auto host_json = json.if_contains("host");
    const auto port_json = json.if_contains("port");
    const auto interval_json = json.if_contains("interval");

    if (!host_json ||
        !port_json ||
        !interval_json) {
        return false;
    }

    if (!host_json->is_string() ||
        !port_json->is_int64() ||
        !interval_json->is_int64()) {
        return false;
    }

    host = host_json->as_string();
    port = static_cast<uint16_t>(port_json->as_int64());
    interval = std::chrono::milliseconds{interval_json->as_int64()};
    return true;
}

bool OrderbookConfig::from_json(const boost::json::object& json)
{
    const auto host_json = json.if_contains("host");
    const auto port_json = json.if_contains("port");
    const auto limit_json = json.if_contains("limit");

    if (!host_json ||
        !port_json ||
        !limit_json) {
        return false;
    }

    if (!host_json->is_string() ||
        !port_json->is_int64() ||
        !limit_json->is_int64()) {
        return false;
    }

    host = host_json->as_string();
    port = static_cast<uint16_t>(port_json->as_int64());
    limit = static_cast<uint16_t>(limit_json->as_int64());
    return true;
}

bool Config::from_json(const boost::json::object& json)
{
    const auto market_json = json.if_contains("market");
    const auto depth_stream_json = json.if_contains("depthStream");
    const auto orderbook_json = json.if_contains("orderbook");
    const auto symbol_json = json.if_contains("symbol");

    if (!market_json ||
        !depth_stream_json ||
        !orderbook_json ||
        !symbol_json) {
        return false;
    }

    if (!market_json->is_string() ||
        !depth_stream_json->is_object() ||
        !orderbook_json->is_object() ||
        !symbol_json->is_string()) {
        return false;
    }

    if (market_from_string(market_json->as_string().c_str()) != Market::BINANCE) {
        return false;
    }

    if (!depth_stream_conf.from_json(depth_stream_json->as_object()) ||
        !orderbook_conf.from_json(orderbook_json->as_object())) {
        return false;
    }

    symbol = symbol_from_string(symbol_json->as_string());
    return true;
}

} // namespace binance
