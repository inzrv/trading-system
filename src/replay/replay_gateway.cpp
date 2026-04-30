#include "replay_gateway.h"

#include "common/log.h"
#include "common/time.h"

#include <utility>

namespace replay
{

ReplayGateway::ReplayGateway(std::shared_ptr<IQueue> queue, MarketData data)
    : m_queue(std::move(queue))
    , m_data(std::move(data))
{}

ReplayGateway::~ReplayGateway()
{
    stop();
}

void ReplayGateway::start()
{
    {
        std::lock_guard lock{m_mutex};
        if (m_state == State::RUNNING) {
            return;
        }

        m_state = State::RUNNING;
    }

    Worker::start();
    m_cv.notify_all();
}

void ReplayGateway::stop()
{
    {
        std::lock_guard lock{m_mutex};
        if (m_state == State::STOPPED) {
            return;
        }

        m_state = State::STOPPED;
    }

    Worker::stop();
    m_cv.notify_all();
}

void ReplayGateway::restart()
{
    stop();
    start();
}

std::expected<std::string, GatewayError> ReplayGateway::request_snapshot()
{
    std::unique_lock lock{m_mutex};
    if (m_next_snapshot_pos >= m_data.snapshots.size()) {
        return std::unexpected(GatewayError::REQUEST_ERROR);
    }

    const auto& snapshot = m_data.snapshots[m_next_snapshot_pos];

    m_cv.wait(lock, [this, &snapshot] {
        return !m_running || m_published_update_idx >= snapshot.requested_after_update_idx;
    });
    if (!m_running) {
        return std::unexpected(GatewayError::REQUEST_ERROR);
    }

    m_snapshot_request_pending = true;
    m_cv.notify_all();

    m_cv.wait(lock, [this, &snapshot] {
        return !m_running || m_published_update_idx >= snapshot.available_after_update_idx;
    });
    if (!m_running) {
        return std::unexpected(GatewayError::REQUEST_ERROR);
    }

    auto payload = snapshot.payload;
    ++m_next_snapshot_pos;
    m_snapshot_request_pending = false;
    m_cv.notify_all();

    return payload;
}

std::expected<void, GatewayError> ReplayGateway::wait_until_running(std::chrono::milliseconds timeout)
{
    std::unique_lock lock{m_mutex};
    const bool running = m_cv.wait_for(lock, timeout, [this] {
        return m_state == State::RUNNING || m_state == State::FAILED;
    });

    if (!running || m_state != State::RUNNING) {
        return std::unexpected(GatewayError::TIMEOUT);
    }

    return {};
}

void ReplayGateway::run()
{
    for (;;) {
        Update update;
        {
            std::unique_lock lock{m_mutex};
            m_cv.wait(lock, [this] {
                return !m_running || !has_next_update() || can_publish_next_update();
            });

            if (!m_running || !has_next_update()) {
                break;
            }

            update = m_data.updates[m_next_update_pos++];
        }

        publish_update(update);

        {
            std::lock_guard lock{m_mutex};
            m_published_update_idx = update.update_idx;
        }
        m_cv.notify_all();
    }

    log::info("ReplayGateway", "finished publishing replay updates");
    if (m_queue) {
        m_queue->close();
    }
}

bool ReplayGateway::can_publish_next_update() const
{
    if (!has_next_update() || m_next_snapshot_pos >= m_data.snapshots.size()) {
        return true;
    }

    if (m_snapshot_request_pending) {
        return true;
    }

    const auto next_update_idx = m_data.updates[m_next_update_pos].update_idx;
    const auto request_barrier = m_data.snapshots[m_next_snapshot_pos].requested_after_update_idx;
    return next_update_idx <= request_barrier;
}

bool ReplayGateway::has_next_update() const
{
    return m_next_update_pos < m_data.updates.size();
}

void ReplayGateway::publish_update(const Update& update)
{
    if (!m_queue) {
        return;
    }

    const bool pushed = m_queue->try_push(InputEnvelope{
        .ingress_time = latency_clock::now(),
        .source = update.source,
        .payload = update.payload,
    });

    if (!pushed) {
        log::warn("ReplayGateway", "failed to publish replay update_idx={}", update.update_idx);
    }
}

} // namespace replay
