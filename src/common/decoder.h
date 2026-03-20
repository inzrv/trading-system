#pragma once

#include "book_update.h"
#include "snapshot.h"
#include "errors.h"

#include <expected>
#include <string_view>

class IDecoder
{
public:
    virtual ~IDecoder() = default;
    virtual std::expected<SequencedBookUpdate, DecodingError> decode_diff(std::string_view diff_payload) const = 0;
    virtual std::expected<Snapshot, DecodingError> decode_snapshot(std::string_view snapshot_payload) const = 0;
};
