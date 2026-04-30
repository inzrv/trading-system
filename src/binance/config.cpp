#include "config.h"

#include "utils/utils.h"

#include <utility>

namespace binance
{

namespace
{

RuntimeMode runtime_mode_from_string(std::string_view mode)
{
    if (mode == "live") {
        return RuntimeMode::LIVE;
    }

    if (mode == "replay") {
        return RuntimeMode::REPLAY;
    }

    return RuntimeMode::LIVE;
}

} // namespace

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

bool ReplayConfig::from_json(const boost::json::object& json)
{
    const auto updates_path_json = json_string(json, "updatesPath");
    const auto snapshots_path_json = json_string(json, "snapshotsPath");

    if (!updates_path_json || !snapshots_path_json) {
        return false;
    }

    updates_path = *updates_path_json;
    snapshots_path = *snapshots_path_json;
    return true;
}

bool LiveConfig::from_json(const boost::json::object& json)
{
    const auto depth_stream_json = json_object(json, "depthStream");
    const auto orderbook_json = json_object(json, "orderbook");
    const auto symbol_json = json_string(json, "symbol");

    if (!depth_stream_json || !orderbook_json || !symbol_json) {
        return false;
    }

    if (!depth_stream_conf.from_json(*depth_stream_json) ||
        !orderbook_conf.from_json(*orderbook_json)) {
        return false;
    }

    symbol = symbol_from_string(*symbol_json);
    return true;
}

bool Config::from_json(const boost::json::object& json)
{
    const auto mode_json = json_string(json, "mode");
    const auto market_json = json_string(json, "market");
    const auto* recording_value = json.if_contains("recording");
    const auto* replay_value = json.if_contains("replay");

    if (!market_json) {
        return false;
    }

    if (mode_json) {
        if (*mode_json != "live" && *mode_json != "replay") {
            return false;
        }
        mode = runtime_mode_from_string(*mode_json);
    } else {
        mode = RuntimeMode::LIVE;
    }

    if (market_from_string(*market_json) != Market::BINANCE) {
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

    if (replay_value) {
        if (!replay_value->is_object()) {
            return false;
        }

        ReplayConfig replay_conf_value;
        if (!replay_conf_value.from_json(replay_value->as_object())) {
            return false;
        }

        replay_conf = std::move(replay_conf_value);
    } else {
        replay_conf.reset();
    }

    if (mode == RuntimeMode::REPLAY && !replay_conf) {
        return false;
    }

    if (mode == RuntimeMode::REPLAY && recording_conf) {
        return false;
    }

    if (mode == RuntimeMode::LIVE) {
        LiveConfig live_conf_value;
        if (!live_conf_value.from_json(json)) {
            return false;
        }

        live_conf = std::move(live_conf_value);
    } else {
        live_conf.reset();
    }

    return true;
}

} // namespace binance
