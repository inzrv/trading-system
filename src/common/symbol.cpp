#include "symbol.h"

Symbol symbol_from_string(std::string_view symbol)
{
    if (symbol == "BTCUSDT") {
        return Symbol::BTCUSDT;
    }

    return Symbol::UNKNOWN;
}

std::string symbol_to_string(Symbol symbol)
{
    switch (symbol) {
        case Symbol::BTCUSDT:
            return "BTCUSDT";
        case Symbol::UNKNOWN:
            break;
    }

    return "UNKNOWN";
}
