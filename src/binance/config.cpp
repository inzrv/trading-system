#include "config.h"

#include "utils/utils.h"

namespace binance
{

bool DepthStreamConfig::from_json(const boost::json::object& json)
{
    const auto host_json = json_string(json, "host");
    const auto port_json = json_int64(json, "port");
    const auto interval_json = json_int64(json, "interval");

    if (!host_json || !port_json || !interval_json) {
        return false;
    }

    host = *host_json;
    port = static_cast<uint16_t>(*port_json);
    interval = std::chrono::milliseconds{*interval_json};
    return true;
}

bool OrderbookConfig::from_json(const boost::json::object& json)
{
    const auto host_json = json_string(json, "host");
    const auto port_json = json_int64(json, "port");
    const auto limit_json = json_int64(json, "limit");

    if (!host_json || !port_json || !limit_json) {
        return false;
    }

    host = *host_json;
    port = static_cast<uint16_t>(*port_json);
    limit = static_cast<uint16_t>(*limit_json);
    return true;
}

bool RecordingConfig::from_json(const boost::json::object& json)
{
    const auto updates_path_json = json_string(json, "updatesPath");
    const auto snapshots_path_json = json_string(json, "snapshotsPath");
    const auto flush_interval_json = json_int64(json, "flushIntervalMs");

    if (!updates_path_json || !snapshots_path_json || !flush_interval_json) {
        return false;
    }

    updates_path = *updates_path_json;
    snapshots_path = *snapshots_path_json;
    flush_interval = std::chrono::milliseconds{*flush_interval_json};

    return true;
}

bool Config::from_json(const boost::json::object& json)
{
    const auto market_json = json_string(json, "market");
    const auto depth_stream_json = json_object(json, "depthStream");
    const auto orderbook_json = json_object(json, "orderbook");
    const auto symbol_json = json_string(json, "symbol");
    const auto* recording_value = json.if_contains("recording");

    if (!market_json || !depth_stream_json || !orderbook_json || !symbol_json) {
        return false;
    }

    if (market_from_string(*market_json) != Market::BINANCE) {
        return false;
    }

    if (!depth_stream_conf.from_json(*depth_stream_json) ||
        !orderbook_conf.from_json(*orderbook_json)) {
        return false;
    }

    if (recording_value) {
        if (!recording_value->is_object()) {
            return false;
        }

        RecordingConfig recording_conf_value;
        if (!recording_conf_value.from_json(recording_value->as_object())) {
            return false;
        }

        recording_conf = std::move(recording_conf_value);
    } else {
        recording_conf.reset();
    }

    symbol = symbol_from_string(*symbol_json);
    return true;
}

} // namespace binance
