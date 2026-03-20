#pragma once

#include "book_update.h"
#include "errors.h"

#include <expected>

class ISequencer
{
public:
    virtual ~ISequencer() = default;
    virtual std::expected<void, SequencingError> check(const SequencedBookUpdate& update) = 0;
    virtual void initialize(uint64_t last_update_id) = 0;
    virtual void reset() = 0;
};
