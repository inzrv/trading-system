#pragma once

#include <string>
#include <string_view>

enum class Market
{
    UNKNOWN,
    BINANCE
};

Market market_from_string(std::string_view market);
std::string market_to_string(Market market);
