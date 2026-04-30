#include "market_data.h"

#include "utils/utils.h"

#include <fstream>
#include <format>

namespace replay
{

namespace
{

std::expected<std::vector<Update>, std::string> load_updates(const std::filesystem::path& path)
{
    std::ifstream input{path};
    if (!input) {
        return std::unexpected(std::format("failed to open replay updates file: {}", path.string()));
    }

    std::vector<Update> updates;
    std::string line;
    size_t line_no{0};
    while (std::getline(input, line)) {
        ++line_no;
        if (line.empty()) {
            continue;
        }

        const auto object = parse_to_json_object(line);
        if (!object) {
            return std::unexpected(std::format("failed to parse {}:{}: line is not a JSON object",
                                               path.string(),
                                               line_no));
        }

        const auto update_idx = json_uint64(*object, "update_idx");
        const auto received_unix_ms = json_int64(*object, "received_unix_ms");
        const auto source = json_string(*object, "source");
        const auto payload = json_string(*object, "payload");
        if (!update_idx || !received_unix_ms || !source || !payload) {
            return std::unexpected(std::format("failed to parse {}:{}: invalid update record",
                                               path.string(),
                                               line_no));
        }

        updates.push_back(Update{
            .update_idx = *update_idx,
            .received_unix_ms = *received_unix_ms,
            .source = *source,
            .payload = *payload,
        });
    }

    return updates;
}

std::expected<std::vector<Snapshot>, std::string> load_snapshots(const std::filesystem::path& path)
{
    std::ifstream input{path};
    if (!input) {
        return std::unexpected(std::format("failed to open replay snapshots file: {}", path.string()));
    }

    std::vector<Snapshot> snapshots;
    std::string line;
    size_t line_no{0};
    while (std::getline(input, line)) {
        ++line_no;
        if (line.empty()) {
            continue;
        }

        const auto object = parse_to_json_object(line);
        if (!object) {
            return std::unexpected(std::format("failed to parse {}:{}: line is not a JSON object",
                                               path.string(),
                                               line_no));
        }

        const auto requested_after_update_idx = json_uint64(*object, "requested_after_update_idx");
        const auto available_after_update_idx = json_uint64(*object, "available_after_update_idx");
        const auto received_unix_ms = json_int64(*object, "received_unix_ms");
        const auto source = json_string(*object, "source");
        const auto payload = json_string(*object, "payload");
        if (!requested_after_update_idx || !available_after_update_idx || !received_unix_ms || !source || !payload) {
            return std::unexpected(std::format("failed to parse {}:{}: invalid snapshot record",
                                               path.string(),
                                               line_no));
        }

        snapshots.push_back(Snapshot{
            .requested_after_update_idx = *requested_after_update_idx,
            .available_after_update_idx = *available_after_update_idx,
            .received_unix_ms = *received_unix_ms,
            .source = *source,
            .payload = *payload,
        });
    }

    return snapshots;
}

} // namespace

std::expected<MarketData, std::string> load_market_data(
    const std::filesystem::path& updates_path,
    const std::filesystem::path& snapshots_path)
{
    auto updates = load_updates(updates_path);
    if (!updates) {
        return std::unexpected(updates.error());
    }

    auto snapshots = load_snapshots(snapshots_path);
    if (!snapshots) {
        return std::unexpected(snapshots.error());
    }

    return MarketData{
        .updates = std::move(*updates),
        .snapshots = std::move(*snapshots),
    };
}

} // namespace replay
