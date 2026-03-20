#pragma once

#include "envelope.h"

#include <optional>

class IQueue
{
public:
    virtual ~IQueue() = default;
    virtual bool try_push(InputEnvelope&& item) = 0;
    virtual std::optional<InputEnvelope> try_pop() = 0;
};
