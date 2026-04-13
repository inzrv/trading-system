#pragma once

#include "gateway.h"
#include "queue.h"
#include "decoder.h"
#include "recovery_manager.h"
#include "sequencer.h"
#include "orderbook.h"
#include "runtime_factory.h"

#include <boost/asio.hpp>
#include <boost/beast/ssl.hpp>

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
    net::io_context m_io_ctx;
    ssl::context m_ssl_ctx;
    net::executor_work_guard<net::io_context::executor_type> m_work_guard;
    std::thread m_io_thread;
    bool m_running{false};

    std::shared_ptr<IQueue> m_queue;
    std::unique_ptr<IGateway> m_gateway;
    std::unique_ptr<IDecoder> m_decoder;
    std::unique_ptr<ISequencer> m_sequencer;
    std::unique_ptr<IOrderbook> m_orderbook;
    std::unique_ptr<IRecoveryManager> m_recovery_manager;
};
