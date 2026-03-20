#pragma once

#include "common/decoder.h"

#include <boost/json.hpp>

namespace binance
{

class Decoder : public IDecoder
{
public:
    std::expected<SequencedBookUpdate, DecodingError> decode_diff(std::string_view diff_payload) const override;
    std::expected<Snapshot, DecodingError> decode_snapshot(std::string_view snapshot_payload) const override;
private:
    std::expected<std::vector<Level>, DecodingError> parse_levels(const boost::json::array& side_updates) const;
};

} // namespace binance
