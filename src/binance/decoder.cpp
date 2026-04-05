#include "decoder.h"

#include "api.h"
#include "common/log.h"
#include "utils/utils.h"

namespace binance
{

std::expected<SequencedBookUpdate, DecodingError>
Decoder::decode_diff(std::string_view diff_payload) const
{   
    const auto parse_to_json_res = parse_to_json(diff_payload);
    if (!parse_to_json_res) {
        log::warn("Decoder", "failed to parse diff payload");
        return std::unexpected(DecodingError::PAYLOAD_PARSING_ERROR);
    }

    boost::json::value json_value = *parse_to_json_res;
    if (!json_value.is_object()) {
        return std::unexpected(DecodingError::INVALID_PAYLOAD);
    }

    const auto& obj = json_value.as_object();

    const auto event_type = obj.if_contains("e");
    const auto event_time = obj.if_contains("E");
    const auto symbol = obj.if_contains("s");
    const auto first_update = obj.if_contains("U");
    const auto last_update = obj.if_contains("u");
    const auto bids = obj.if_contains("b");
    const auto asks = obj.if_contains("a");

    if (!event_type ||
        !event_time ||
        !symbol ||
        !first_update ||
        !last_update ||
        !bids ||
        !asks) {
        return std::unexpected(DecodingError::INVALID_PAYLOAD);
    }

    if (!event_type->is_string() ||
        !symbol->is_string() ||
        !event_time->is_int64() ||
        !first_update->is_int64() ||
        !last_update->is_int64() ||
        !bids->is_array() ||
        !asks->is_array()) {
        return std::unexpected(DecodingError::INVALID_PAYLOAD);
    }

    if (auto type = event_type_from_string(event_type->as_string());  type != EventType::DEPTH_UPDATE) {
        return std::unexpected(DecodingError::UNEXPECTED_VALUE);
    }

    auto bids_res = parse_levels(bids->as_array());
    if (!bids_res) {
        return std::unexpected(bids_res.error());
    }

    auto asks_res = parse_levels(asks->as_array());
    if (!asks_res) {
        return std::unexpected(asks_res.error());
    }

    SequencedBookUpdate update = {
        .event_time = std::chrono::system_clock::time_point{std::chrono::milliseconds{event_time->as_int64()}},
        .symbol = symbol_from_string(symbol->as_string()),
        .first_update = static_cast<uint64_t>(first_update->as_int64()),
        .last_update = static_cast<uint64_t>(last_update->as_int64()),
        .bids = std::move(bids_res.value()),
        .asks = std::move(asks_res.value()),
    };

    return update;
}

std::expected<Snapshot, DecodingError>
Decoder::decode_snapshot(std::string_view snapshot_payload) const
{
    const auto parse_to_json_res = parse_to_json(snapshot_payload);
    if (!parse_to_json_res) {
        log::warn("Decoder", "failed to parse snapshot payload");
        return std::unexpected(DecodingError::PAYLOAD_PARSING_ERROR);
    }

    boost::json::value json_value = *parse_to_json_res;
    if (!json_value.is_object()) {
        return std::unexpected(DecodingError::INVALID_PAYLOAD);
    }

    const auto& obj = json_value.as_object();

    const auto last_update_id = obj.if_contains("lastUpdateId");
    const auto bids = obj.if_contains("bids");
    const auto asks = obj.if_contains("asks");

    if (!last_update_id ||
        !bids ||
        !asks) {
        return std::unexpected(DecodingError::INVALID_PAYLOAD);
    }

    if (!last_update_id->is_int64() ||
        !bids->is_array() ||
        !asks->is_array()) {
        return std::unexpected(DecodingError::INVALID_PAYLOAD);
    }

    auto bids_res = parse_levels(bids->as_array());
    if (!bids_res) {
        return std::unexpected(bids_res.error());
    }

    auto asks_res = parse_levels(asks->as_array());
    if (!asks_res) {
        return std::unexpected(asks_res.error());
    }

    Snapshot snapshot = {
        .last_update_id = static_cast<uint64_t>(last_update_id->as_int64()),
        .bids = std::move(bids_res.value()),
        .asks = std::move(asks_res.value()),
    };

    return snapshot;
}

std::expected<std::vector<Level>, DecodingError>
Decoder::parse_levels(const boost::json::array& side_updates) const
{
    std::vector<Level> out;
    out.reserve(side_updates.size());

    for (const auto& entry : side_updates) {
        if (!entry.is_array()) {
            log::warn("Decoder", "invalid level payload: entry is not array");
            return std::unexpected(DecodingError::INVALID_PAYLOAD);
        }

        const auto& level = entry.as_array();
        if (level.size() != 2 || !level[0].is_string() || !level[1].is_string()) {
            log::warn("Decoder", "invalid level payload: expected [price,quantity]");
            return std::unexpected(DecodingError::INVALID_PAYLOAD);
        }

        try {
            out.push_back(Level{
                .price = std::stod(std::string(level[0].as_string().c_str())),
                .quantity = std::stod(std::string(level[1].as_string().c_str()))
            });
        } catch (const std::exception&) {
            return std::unexpected(DecodingError::INVALID_PAYLOAD);
        }
    }

    return out;
}

} // namespace binance
