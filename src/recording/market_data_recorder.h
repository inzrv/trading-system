#pragma once

#include "common/worker.h"

#include <chrono>
#include <cstdint>
#include <deque>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <variant>

namespace recording
{

struct RecordedUpdate
{
    uint64_t update_idx{0};
    int64_t received_unix_ms{0};
    std::string source;
    std::string payload;
};

struct RecordedSnapshot
{
    uint64_t requested_after_update_idx{0};
    uint64_t available_after_update_idx{0};
    int64_t received_unix_ms{0};
    std::string source;
    std::string payload;
};

using RecordedEvent = std::variant<RecordedUpdate, RecordedSnapshot>;

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
    void write_batch(std::deque<RecordedEvent>& batch);
    void write_update(const RecordedUpdate& record);
    void write_snapshot(const RecordedSnapshot& record);

private:
    std::filesystem::path m_updates_path;
    std::filesystem::path m_snapshots_path;
    std::chrono::milliseconds m_flush_interval;
    std::ofstream m_updates_output;
    std::ofstream m_snapshots_output;

    std::deque<RecordedEvent> m_queue;
    uint64_t m_last_update_idx{0};
};

} // namespace recording
