#include "market_data_recorder.h"

#include "common/log.h"
#include "utils/utils.h"

#include <boost/json.hpp>

#include <format>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace recording
{

namespace
{

std::ofstream open_output_file(const std::filesystem::path& path, std::string_view label)
{
    if (path.empty()) {
        throw std::invalid_argument(std::format("{} recorder output path is empty", label));
    }

    if (path.has_parent_path()) {
        std::filesystem::create_directories(path.parent_path());
    }

    std::ofstream output{path, std::ios::binary | std::ios::trunc};
    if (!output) {
        throw std::runtime_error(std::format("failed to open {} recorder output file: {}", label, path.string()));
    }

    return output;
}

} // namespace

MarketDataRecorder::MarketDataRecorder(std::filesystem::path updates_path,
                                       std::filesystem::path snapshots_path,
                                       std::chrono::milliseconds flush_interval)
    : m_updates_path(std::move(updates_path))
    , m_snapshots_path(std::move(snapshots_path))
    , m_flush_interval(flush_interval)
{
    m_updates_output = open_output_file(m_updates_path, "update");
    m_snapshots_output = open_output_file(m_snapshots_path, "snapshot");
}

MarketDataRecorder::~MarketDataRecorder()
{
    stop();
}

void MarketDataRecorder::record_update(std::string_view source, std::string_view payload)
{
    RecordedUpdate record{
        .received_unix_ms = now_unix_ms(),
        .source = std::string(source),
        .payload = std::string(payload),
    };

    {
        std::lock_guard lock{m_mutex};
        if (!m_running) {
            return;
        }

        record.update_idx = ++m_last_update_idx;
        m_queue.push_back(std::move(record));
    }
    m_cv.notify_one();
}

uint64_t MarketDataRecorder::last_update_idx()
{
    std::lock_guard lock{m_mutex};
    return m_last_update_idx;
}

void MarketDataRecorder::record_snapshot(uint64_t requested_after_update_idx,
                                         std::string_view source,
                                         std::string_view payload)
{
    RecordedSnapshot record{
        .requested_after_update_idx = requested_after_update_idx,
        .received_unix_ms = now_unix_ms(),
        .source = std::string(source),
        .payload = std::string(payload),
    };

    {
        std::lock_guard lock{m_mutex};
        if (!m_running) {
            return;
        }

        record.available_after_update_idx = m_last_update_idx;
        m_queue.push_back(std::move(record));
    }
    m_cv.notify_one();
}

void MarketDataRecorder::run()
{
    std::deque<RecordedEvent> batch;

    for (;;) {
        {
            std::unique_lock lock{m_mutex};
            m_cv.wait_for(lock, m_flush_interval, [this] {
                return !m_running || !m_queue.empty();
            });

            if (m_queue.empty() && !m_running) {
                break;
            }

            if (m_queue.empty()) {
                continue;
            }

            batch.swap(m_queue);
        }

        write_batch(batch);
        batch.clear();
    }

    if (!batch.empty()) {
        write_batch(batch);
    }

    m_updates_output.flush();
    m_snapshots_output.flush();
}

void MarketDataRecorder::write_batch(std::deque<RecordedEvent>& batch)
{
    for (const auto& record : batch) {
        std::visit([this](const auto& typed_record) {
            using T = std::decay_t<decltype(typed_record)>;
            if constexpr (std::is_same_v<T, RecordedUpdate>) {
                write_update(typed_record);
            } else {
                write_snapshot(typed_record);
            }
        }, record);
    }

    m_updates_output.flush();
    m_snapshots_output.flush();
}

void MarketDataRecorder::write_update(const RecordedUpdate& record)
{
    boost::json::object json_record;
    json_record["type"] = "ws_update";
    json_record["update_idx"] = record.update_idx;
    json_record["received_unix_ms"] = record.received_unix_ms;
    json_record["source"] = record.source;
    json_record["payload"] = record.payload;

    m_updates_output << boost::json::serialize(json_record) << '\n';
}

void MarketDataRecorder::write_snapshot(const RecordedSnapshot& record)
{
    boost::json::object json_record;
    json_record["type"] = "snapshot";
    json_record["requested_after_update_idx"] = record.requested_after_update_idx;
    json_record["available_after_update_idx"] = record.available_after_update_idx;
    json_record["received_unix_ms"] = record.received_unix_ms;
    json_record["source"] = record.source;
    json_record["payload"] = record.payload;

    m_snapshots_output << boost::json::serialize(json_record) << '\n';
}

} // namespace recording
