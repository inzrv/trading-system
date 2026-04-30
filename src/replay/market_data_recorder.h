#pragma once

#include "common/worker.h"
#include "market_data.h"

#include <chrono>
#include <cstdint>
#include <deque>
#include <filesystem>
#include <fstream>
#include <string_view>

namespace replay
{

class MarketDataRecorder final : public Worker
{
public:
    MarketDataRecorder(std::filesystem::path updates_path,
                       std::filesystem::path snapshots_path,
                       std::chrono::milliseconds flush_interval);

    ~MarketDataRecorder();

    void record_update(std::string_view source, std::string_view payload);
    [[nodiscard]] uint64_t last_update_idx();
    void record_snapshot(uint64_t requested_after_update_idx, std::string_view source, std::string_view payload);

private:
    void run() override;
    void write_batch(std::deque<Event>& batch);
    void write_update(const Update& update);
    void write_snapshot(const Snapshot& update);

private:
    std::filesystem::path m_updates_path;
    std::filesystem::path m_snapshots_path;
    std::chrono::milliseconds m_flush_interval;
    std::ofstream m_updates_output;
    std::ofstream m_snapshots_output;

    std::deque<Event> m_queue;
    uint64_t m_last_update_idx{0};
};

} // namespace replay
