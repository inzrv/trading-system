#pragma once

#include "gateway.h"
#include "decoder.h"
#include "metrics/reporter.h"
#include "metrics/registry.h"
#include "recovery_manager.h"
#include "recording/market_data_recorder.h"
#include "sequencer.h"
#include "queue.h"
#include "orderbook.h"
#include "runtime_factory.h"

#include <boost/asio.hpp>
#include <boost/beast/ssl.hpp>

#include <atomic>
#include <chrono>
#include <expected>
#include <memory>
#include <thread>

namespace net = boost::asio;
namespace ssl = net::ssl;

class Runtime
{
public:
    explicit Runtime(IRuntimeFactory& factory);
    ~Runtime();

    std::expected<void, RuntimeError> run();
    void stop();

private:
    std::expected<void, RuntimeError> run_core_loop();

private:
    static constexpr std::chrono::seconds kMetricsReportInterval{5};

    net::io_context m_io_ctx;
    ssl::context m_ssl_ctx;
    net::executor_work_guard<net::io_context::executor_type> m_work_guard;
    std::thread m_io_thread;
    std::atomic<bool> m_running{false};

    std::unique_ptr<metrics::Registry> m_metrics;
    std::unique_ptr<metrics::Reporter> m_metrics_reporter;
    std::unique_ptr<recording::MarketDataRecorder> m_market_data_recorder;
    std::shared_ptr<IQueue> m_queue;
    std::unique_ptr<IGateway> m_gateway;
    std::unique_ptr<IDecoder> m_decoder;
    std::unique_ptr<ISequencer> m_sequencer;
    std::unique_ptr<IOrderbook> m_orderbook;
    std::unique_ptr<IRecoveryManager> m_recovery_manager;
};
