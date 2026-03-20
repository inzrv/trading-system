#pragma once

#include <map>
#include <string_view>
#include <optional>

enum class Symbol
{
    UNKNOWN,
    BTCUSDT
};

Symbol symbol_from_string(std::string_view symbol);
