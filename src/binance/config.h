#pragma once

#include "common/config.h"
#include "common/symbol.h"

#include <array>
#include <chrono>
#include <cstdint>
#include <optional>
#include <string>

namespace binance
{

enum class RuntimeMode
{
    LIVE,
    REPLAY
};

struct DepthStreamConfig
{
    bool from_json(const boost::json::object& json);

    std::string host;
    uint16_t port{0};
    std::chrono::milliseconds interval{0};

private:
    static constexpr std::array<std::chrono::milliseconds, 2> kAllowedIntervals{
        std::chrono::milliseconds{100},
        std::chrono::milliseconds{1000}
    };
};

struct OrderbookConfig
{
    bool from_json(const boost::json::object& json);

    std::string host;
    uint16_t port{0};
    uint16_t limit{0};
};

struct RecordingConfig
{
    bool from_json(const boost::json::object& json);

    std::string updates_path;
    std::string snapshots_path;
    std::chrono::milliseconds flush_interval{250};
};

struct ReplayConfig
{
    bool from_json(const boost::json::object& json);

    std::string updates_path;
    std::string snapshots_path;
};

struct LiveConfig
{
    bool from_json(const boost::json::object& json);

    Symbol symbol{Symbol::UNKNOWN};
    DepthStreamConfig depth_stream_conf;
    OrderbookConfig orderbook_conf;
};

struct Config final : public IConfig
{
    Market market() const noexcept override
    {
        return Market::BINANCE;
    }

    bool from_json(const boost::json::object& json) override;

    RuntimeMode mode{RuntimeMode::LIVE};
    std::optional<LiveConfig> live_conf;
    std::optional<RecordingConfig> recording_conf;
    std::optional<ReplayConfig> replay_conf;
};

} // namespace binance
