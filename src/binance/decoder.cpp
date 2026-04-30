#include "decoder.h"

#include "api.h"
#include "common/log.h"
#include "utils/utils.h"

namespace binance
{

std::expected<SequencedBookUpdate, DecodingError>
Decoder::decode_diff(std::string_view diff_payload) const
{   
    const auto json_object_res = parse_to_json_object(diff_payload);
    if (!json_object_res) {
        log::warn("Decoder", "failed to parse diff payload");
        return std::unexpected(DecodingError::PAYLOAD_PARSING_ERROR);
    }

    const auto& obj = *json_object_res;

    const auto event_type = json_string(obj, "e");
    const auto event_time = json_int64(obj, "E");
    const auto symbol = json_string(obj, "s");
    const auto first_update = json_uint64(obj, "U");
    const auto last_update = json_uint64(obj, "u");
    const auto bids = json_array(obj, "b");
    const auto asks = json_array(obj, "a");

    if (!event_type || !event_time || !symbol || !first_update || !last_update || !bids || !asks) {
        return std::unexpected(DecodingError::INVALID_PAYLOAD);
    }

    if (auto type = event_type_from_string(*event_type);  type != EventType::DEPTH_UPDATE) {
        return std::unexpected(DecodingError::UNEXPECTED_VALUE);
    }

    if (*first_update > *last_update) {
        return std::unexpected(DecodingError::INVALID_PAYLOAD);
    }

    auto bids_res = parse_levels(*bids);
    if (!bids_res) {
        return std::unexpected(bids_res.error());
    }

    auto asks_res = parse_levels(*asks);
    if (!asks_res) {
        return std::unexpected(asks_res.error());
    }

    SequencedBookUpdate update = {
        .event_time = std::chrono::system_clock::time_point{std::chrono::milliseconds{*event_time}},
        .symbol = symbol_from_string(*symbol),
        .first_update = *first_update,
        .last_update = *last_update,
        .bids = std::move(bids_res.value()),
        .asks = std::move(asks_res.value()),
    };

    return update;
}

std::expected<Snapshot, DecodingError>
Decoder::decode_snapshot(std::string_view snapshot_payload) const
{
    const auto json_object_res = parse_to_json_object(snapshot_payload);
    if (!json_object_res) {
        log::warn("Decoder", "failed to parse snapshot payload");
        return std::unexpected(DecodingError::PAYLOAD_PARSING_ERROR);
    }

    const auto& obj = *json_object_res;

    const auto last_update_id = json_uint64(obj, "lastUpdateId");
    const auto bids = json_array(obj, "bids");
    const auto asks = json_array(obj, "asks");

    if (!last_update_id || !bids || !asks) {
        return std::unexpected(DecodingError::INVALID_PAYLOAD);
    }

    auto bids_res = parse_levels(*bids);
    if (!bids_res) {
        return std::unexpected(bids_res.error());
    }

    auto asks_res = parse_levels(*asks);
    if (!asks_res) {
        return std::unexpected(asks_res.error());
    }

    Snapshot snapshot = {
        .last_update_id = *last_update_id,
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
