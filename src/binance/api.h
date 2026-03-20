#pragma once

#include <map>
#include <string_view>

namespace binance
{

enum class EventType
{
    UNKNOWN,
    DEPTH_UPDATE
};

EventType event_type_from_string(std::string_view event_type);

} // namespace binance
