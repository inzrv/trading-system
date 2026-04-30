#pragma once

#include "common/gateway.h"
#include "common/queue.h"
#include "common/worker.h"
#include "market_data.h"

#include <chrono>
#include <memory>

namespace replay
{

class ReplayGateway final : public IGateway, private Worker
{
public:
    ReplayGateway(std::shared_ptr<IQueue> queue, MarketData data);
    ~ReplayGateway() override;

    void open() override;
    void close() override;
    void reopen() override;
    std::expected<std::string, GatewayError> request_snapshot() override;
    std::expected<void, GatewayError> wait_until_ready(std::chrono::milliseconds timeout) override;

private:
    void run() override;
    [[nodiscard]] bool can_publish_next_update() const;
    [[nodiscard]] bool has_next_update() const;
    void publish_update(const Update& update);

private:
    std::shared_ptr<IQueue> m_queue;
    MarketData m_data;

    size_t m_next_update_pos{0};
    size_t m_next_snapshot_pos{0};
    uint64_t m_published_update_idx{0};
    bool m_snapshot_request_pending{false};
};

} // namespace replay
