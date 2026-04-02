#pragma once

#include "decoder.h"
#include "gateway.h"
#include "orderbook.h"
#include "queue.h"
#include "recovery_manager.h"
#include "sequencer.h"

#include <boost/asio/io_context.hpp>
#include <boost/asio/ssl/context.hpp>

#include <memory>

struct RuntimeComponents
{
    std::shared_ptr<IQueue> queue;
    std::unique_ptr<IGateway> gateway;
    std::unique_ptr<IDecoder> decoder;
    std::unique_ptr<ISequencer> sequencer;
    std::unique_ptr<IOrderbook> orderbook;
    std::unique_ptr<IRecoveryManager> recovery_manager;
};

class IRuntimeFactory
{
public:
    virtual ~IRuntimeFactory() = default;

    virtual RuntimeComponents create(boost::asio::io_context& io_ctx,
                                     boost::asio::ssl::context& ssl_ctx) = 0;
};
