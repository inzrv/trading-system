#pragma once

#include <chrono>

using latency_clock = std::chrono::steady_clock;
using latency_time_point = latency_clock::time_point;

inline std::chrono::nanoseconds elapsed_since(latency_time_point started_at) noexcept
{
    return std::chrono::duration_cast<std::chrono::nanoseconds>(latency_clock::now() - started_at);
}
