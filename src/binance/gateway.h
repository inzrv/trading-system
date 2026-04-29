#pragma once

#include "common/gateway.h"
#include "common/queue.h"

#include "network/ws_source.h"
#include "network/rest_client.h"
#include "config.h"
#include "metrics/registry.h"
#include "recording/market_data_recorder.h"

#include <memory>
#include <chrono>

namespace binance
{

class Gateway : public IGateway
{
public:
    Gateway(Config config,
            net::io_context& io_ctx,
            ssl::context& ssl_ctx,
            std::shared_ptr<IQueue> queue,
            metrics::Registry& metrics,
            recording::MarketDataRecorder* market_data_recorder = nullptr);

    void start() override;
    void stop() override;
    void restart() override;
    std::expected<std::string, GatewayError> request_snapshot() override;
    std::expected<void, GatewayError> wait_until_running(std::chrono::milliseconds timeout) override;

private:
    void on_ws_state(WsSource::State state);
    void on_ws_error(beast::error_code ec, std::string_view where);

private:
    Config m_config;
    net::io_context& m_io_ctx;
    ssl::context& m_ssl_ctx;
    std::shared_ptr<IQueue> m_queue;
    metrics::Registry& m_metrics;
    recording::MarketDataRecorder* m_market_data_recorder{nullptr};
    std::unique_ptr<WsSource> m_ws_source;
    std::unique_ptr<RestClient> m_rest_client;

    mutable std::mutex m_state_mutex;
    std::condition_variable m_state_cv;

    static constexpr int kSnapshotMaxAttempts{3};
    static constexpr std::chrono::milliseconds kSnapshotBaseBackoff{200};
};
} // namespace binance
