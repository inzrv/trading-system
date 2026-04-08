#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "binance/recovery_manager.h"
#include "common/gateway.h"
#include "common/decoder.h"
#include "common/sequencer.h"
#include "common/orderbook.h"

using ::testing::_;
using ::testing::Return;

namespace
{
class MockGateway : public IGateway
{
public:
    MOCK_METHOD(void, start, (), (override));
    MOCK_METHOD(void, stop, (), (override));
    MOCK_METHOD(void, restart, (), (override));
    MOCK_METHOD((std::expected<std::string, GatewayError>), request_snapshot, (), (override));
    MOCK_METHOD((std::expected<void, GatewayError>), wait_until_running, (std::chrono::milliseconds), (override));
};

class MockDecoder : public IDecoder
{
public:
    MOCK_METHOD((std::expected<SequencedBookUpdate, DecodingError>), decode_diff, (std::string_view), (const, override));
    MOCK_METHOD((std::expected<Snapshot, DecodingError>), decode_snapshot, (std::string_view), (const, override));
};

class MockSequencer : public ISequencer
{
public:
    MOCK_METHOD((std::expected<void, SequencingError>), check, (const SequencedBookUpdate&), (override));
    MOCK_METHOD(void, initialize, (uint64_t), (override));
    MOCK_METHOD(void, reset, (), (override));
};

class MockOrderbook : public IOrderbook
{
public:
    MOCK_METHOD(void, apply, (SequencedBookUpdate), (override));
    MOCK_METHOD(void, initialize, (Snapshot), (override));
    MOCK_METHOD(void, reset, (), (override));
    MOCK_METHOD(L1, l1, (), (const, override));
};

TEST(RecoveryManagerTest, BeginInitializeSuccess)
{
    MockGateway gateway;
    MockDecoder decoder;
    MockSequencer sequencer;
    MockOrderbook orderbook;

    EXPECT_CALL(orderbook, reset());
    EXPECT_CALL(sequencer, reset());
    EXPECT_CALL(gateway, start());
    EXPECT_CALL(gateway, wait_until_running(_)).WillOnce(Return(std::expected<void, GatewayError>{}));

    binance::RecoveryManager recovery_manager(gateway, decoder, sequencer, orderbook);
    const auto init_res = recovery_manager.begin_initialize();

    ASSERT_TRUE(init_res.has_value());
    EXPECT_TRUE(recovery_manager.is_recovering());
}

TEST(RecoveryManagerTest, BeginInitializeFailsWhenGatewayNotRunning)
{
    MockGateway gateway;
    MockDecoder decoder;
    MockSequencer sequencer;
    MockOrderbook orderbook;

    EXPECT_CALL(orderbook, reset());
    EXPECT_CALL(sequencer, reset());
    EXPECT_CALL(gateway, start());
    EXPECT_CALL(gateway, wait_until_running(_))
        .WillOnce(Return(std::unexpected(GatewayError::TIMEOUT)));

    binance::RecoveryManager recovery_manager(gateway, decoder, sequencer, orderbook);
    const auto init_res = recovery_manager.begin_initialize();

    ASSERT_FALSE(init_res.has_value());
    EXPECT_EQ(init_res.error(), RecoveringError::GATEWAY_START_ERROR);
}

} // namespace
