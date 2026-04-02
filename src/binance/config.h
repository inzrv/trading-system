#pragma once

#include "common/config.h"
#include "common/symbol.h"

#include <array>
#include <chrono>
#include <cstdint>
#include <string>

namespace binance
{

struct DepthStreamConfig
{
    bool from_json(const boost::json::object& json);

    std::string host;
    uint16_t port;
    std::chrono::milliseconds interval;

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
    uint16_t port;
    uint16_t limit;
};

struct Config final : public IConfig
{
    Market market() const noexcept override
    {
        return Market::BINANCE;
    }

    bool from_json(const boost::json::object& json) override;

    Symbol symbol;
    DepthStreamConfig depth_stream_conf;
    OrderbookConfig orderbook_conf;
};

} // namespace binance
