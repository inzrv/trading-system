#pragma once

#include "book_update.h"
#include "snapshot.h"

struct L1
{
    std::optional<Level> best_bid;
    std::optional<Level> best_ask;
};

class IOrderbook
{
public:
    virtual ~IOrderbook() = default;
    virtual void apply(SequencedBookUpdate update) = 0;
    virtual void initialize(Snapshot snapshot) = 0;
    virtual void reset() = 0;
    virtual L1 l1() const = 0;
};