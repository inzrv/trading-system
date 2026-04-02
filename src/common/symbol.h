#pragma once

#include <map>
#include <string>
#include <string_view>
#include <optional>

enum class Symbol
{
    UNKNOWN,
    BTCUSDT
};

Symbol symbol_from_string(std::string_view symbol);
std::string symbol_to_string(Symbol symbol);
