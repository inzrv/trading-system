#pragma once

#include <string>
#include <chrono>

struct InputEnvelope
{
    std::chrono::system_clock::time_point timestamp;
    std::string source;
    std::string payload;
};
