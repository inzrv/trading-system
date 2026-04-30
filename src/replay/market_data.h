#pragma once

#include <cstdint>
#include <expected>
#include <filesystem>
#include <string>
#include <variant>
#include <vector>

namespace replay
{

struct Update
{
    uint64_t update_idx{0};
    int64_t received_unix_ms{0};
    std::string source;
    std::string payload;
};

struct Snapshot
{
    uint64_t requested_after_update_idx{0};
    uint64_t available_after_update_idx{0};
    int64_t received_unix_ms{0};
    std::string source;
    std::string payload;
};

using Event = std::variant<Update, Snapshot>;

struct MarketData
{
    std::vector<Update> updates;
    std::vector<Snapshot> snapshots;
};

std::expected<MarketData, std::string> load_market_data(
    const std::filesystem::path& updates_path,
    const std::filesystem::path& snapshots_path);

} // namespace replay
