#include "market.h"

Market market_from_string(std::string_view market)
{
    if (market == "binance" || market == "BINANCE") {
        return Market::BINANCE;
    }

    return Market::UNKNOWN;
}

std::string market_to_string(Market market)
{
    switch (market) {
        case Market::BINANCE:
            return "binance";
        case Market::UNKNOWN:
            break;
    }

    return "unknown";
}
