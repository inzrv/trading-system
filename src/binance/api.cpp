#include "api.h"

namespace binance
{

EventType event_type_from_string(std::string_view event_type)
{
    if (event_type == "depthUpdate") {
        return EventType::DEPTH_UPDATE;
    }

    return EventType::UNKNOWN;
}

} // namespace binance
