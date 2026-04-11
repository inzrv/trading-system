#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "binance/recovery_manager.h"
#include "common/gateway.h"
#include "common/decoder.h"
#include "common/sequencer.h"
#include "common/orderbook.h"

using ::testing::_;
using ::testing::AllOf;
using ::testing::Field;
using ::testing::Return;

namespace
{
constexpr uint64_t kLastUpdateId = 100;

const std::string kValidBinanceSnapshot =
    R"({"lastUpdateId":100,"bids":[],"asks":[]})";

Snapshot make_snapshot(uint64_t last_update_id = kLastUpdateId)
{
    return Snapshot{
        .last_update_id = last_update_id,
        .bids = {},
        .asks = {},
    };
}

SequencedBookUpdate make_update(uint64_t update_id = kLastUpdateId + 1)
{
    return SequencedBookUpdate{
        .event_time = {},
        .symbol = Symbol::BTCUSDT,
        .first_update = update_id,
        .last_update = update_id + 1,
        .bids = {},
        .asks = {}
    };
}

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

TEST(RecoveryManagerTest, BeginInitializeFails)
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

TEST(RecoveryManagerTest, TryRecoverLoadsSnapshot)
{
    MockGateway gateway;
    MockDecoder decoder;
    MockSequencer sequencer;
    MockOrderbook orderbook;

    EXPECT_CALL(orderbook, reset()).Times(1);
    EXPECT_CALL(sequencer, reset()).Times(1);
    EXPECT_CALL(gateway, start()).Times(1);
    EXPECT_CALL(gateway, wait_until_running(_))
        .WillOnce(Return(std::expected<void, GatewayError>{}));

    EXPECT_CALL(gateway, request_snapshot())
        .Times(1)
        .WillOnce(Return(std::expected<std::string, GatewayError>{kValidBinanceSnapshot}));

    EXPECT_CALL(decoder, decode_snapshot(std::string_view{kValidBinanceSnapshot}))
        .Times(1)
        .WillOnce(Return(std::expected<Snapshot, DecodingError>{make_snapshot()}));

    // Recovery cannot continue yet because there is no buffered update that
    // covers snapshot.last_update_id + 1.
    EXPECT_CALL(orderbook, initialize(_)).Times(0);
    EXPECT_CALL(orderbook, apply(_)).Times(0);
    EXPECT_CALL(sequencer, initialize(_)).Times(0);
    EXPECT_CALL(sequencer, check(_)).Times(0);

    binance::RecoveryManager recovery_manager(gateway, decoder, sequencer, orderbook);

    ASSERT_TRUE(recovery_manager.begin_initialize().has_value());

    const auto recover_res = recovery_manager.try_recover();

    ASSERT_TRUE(recover_res.has_value());
    EXPECT_TRUE(recovery_manager.is_recovering());
}

TEST(RecoveryManagerTest, TryRecoverRequestsSnapshotOnce)
{
    MockGateway gateway;
    MockDecoder decoder;
    MockSequencer sequencer;
    MockOrderbook orderbook;

    EXPECT_CALL(orderbook, reset()).Times(1);
    EXPECT_CALL(sequencer, reset()).Times(1);
    EXPECT_CALL(gateway, start()).Times(1);
    EXPECT_CALL(gateway, wait_until_running(_))
        .WillOnce(Return(std::expected<void, GatewayError>{}));

    EXPECT_CALL(gateway, request_snapshot())
        .Times(1)
        .WillOnce(Return(std::expected<std::string, GatewayError>{kValidBinanceSnapshot}));

    EXPECT_CALL(decoder, decode_snapshot(std::string_view{kValidBinanceSnapshot}))
        .Times(1)
        .WillOnce(Return(std::expected<Snapshot, DecodingError>{make_snapshot()}));

    EXPECT_CALL(orderbook, initialize(_)).Times(0);
    EXPECT_CALL(orderbook, apply(_)).Times(0);
    EXPECT_CALL(sequencer, initialize(_)).Times(0);
    EXPECT_CALL(sequencer, check(_)).Times(0);

    binance::RecoveryManager recovery_manager(gateway, decoder, sequencer, orderbook);

    ASSERT_TRUE(recovery_manager.begin_initialize().has_value());

    ASSERT_TRUE(recovery_manager.try_recover().has_value());
    ASSERT_TRUE(recovery_manager.try_recover().has_value());

    EXPECT_TRUE(recovery_manager.is_recovering());
}

TEST(RecoveryManagerTest, TryRecoverRequestFails)
{
    MockGateway gateway;
    MockDecoder decoder;
    MockSequencer sequencer;
    MockOrderbook orderbook;

    EXPECT_CALL(orderbook, reset()).Times(1);
    EXPECT_CALL(sequencer, reset()).Times(1);
    EXPECT_CALL(gateway, start()).Times(1);
    EXPECT_CALL(gateway, wait_until_running(_))
        .WillOnce(Return(std::expected<void, GatewayError>{}));

    EXPECT_CALL(gateway, request_snapshot())
        .Times(1)
        .WillOnce(Return(std::unexpected(GatewayError::REQUEST_ERROR)));

    EXPECT_CALL(decoder, decode_snapshot(_)).Times(0);
    EXPECT_CALL(orderbook, initialize(_)).Times(0);
    EXPECT_CALL(orderbook, apply(_)).Times(0);
    EXPECT_CALL(sequencer, initialize(_)).Times(0);
    EXPECT_CALL(sequencer, check(_)).Times(0);

    binance::RecoveryManager recovery_manager(gateway, decoder, sequencer, orderbook);

    ASSERT_TRUE(recovery_manager.begin_initialize().has_value());

    const auto recover_res = recovery_manager.try_recover();

    ASSERT_FALSE(recover_res.has_value());
    EXPECT_EQ(recover_res.error(), RecoveringError::SNAPSHOT_REQUEST_ERROR);
    EXPECT_TRUE(recovery_manager.is_recovering());
}

TEST(RecoveryManagerTest, TryRecoverParseFails)
{
    MockGateway gateway;
    MockDecoder decoder;
    MockSequencer sequencer;
    MockOrderbook orderbook;

    EXPECT_CALL(orderbook, reset()).Times(1);
    EXPECT_CALL(sequencer, reset()).Times(1);
    EXPECT_CALL(gateway, start()).Times(1);
    EXPECT_CALL(gateway, wait_until_running(_))
        .WillOnce(Return(std::expected<void, GatewayError>{}));

    EXPECT_CALL(gateway, request_snapshot())
        .Times(1)
        .WillOnce(Return(std::expected<std::string, GatewayError>{kValidBinanceSnapshot}));

    EXPECT_CALL(decoder, decode_snapshot(std::string_view{kValidBinanceSnapshot}))
        .Times(1)
        .WillOnce(Return(std::unexpected(DecodingError::INVALID_PAYLOAD)));

    EXPECT_CALL(orderbook, initialize(_)).Times(0);
    EXPECT_CALL(orderbook, apply(_)).Times(0);
    EXPECT_CALL(sequencer, initialize(_)).Times(0);
    EXPECT_CALL(sequencer, check(_)).Times(0);

    binance::RecoveryManager recovery_manager(gateway, decoder, sequencer, orderbook);

    ASSERT_TRUE(recovery_manager.begin_initialize().has_value());

    const auto recover_res = recovery_manager.try_recover();

    ASSERT_FALSE(recover_res.has_value());
    EXPECT_EQ(recover_res.error(), RecoveringError::SNAPSHOT_PARSING_ERROR);
    EXPECT_TRUE(recovery_manager.is_recovering());
}

TEST(RecoveryManagerTest, TryRecover)
{
    MockGateway gateway;
    MockDecoder decoder;
    MockSequencer sequencer;
    MockOrderbook orderbook;
    Snapshot snapshot = make_snapshot();
    SequencedBookUpdate update = make_update();
    const auto update_matcher = AllOf(
        Field(&SequencedBookUpdate::symbol, Symbol::BTCUSDT),
        Field(&SequencedBookUpdate::first_update, update.first_update),
        Field(&SequencedBookUpdate::last_update, update.last_update));

    EXPECT_CALL(orderbook, reset()).Times(2);
    EXPECT_CALL(sequencer, reset()).Times(2);
    EXPECT_CALL(gateway, start()).Times(1);
    EXPECT_CALL(gateway, wait_until_running(_))
        .WillOnce(Return(std::expected<void, GatewayError>{}));

    EXPECT_CALL(gateway, request_snapshot())
        .Times(1)
        .WillOnce(Return(std::expected<std::string, GatewayError>{kValidBinanceSnapshot}));

    EXPECT_CALL(decoder, decode_snapshot(std::string_view{kValidBinanceSnapshot}))
        .Times(1)
        .WillOnce(Return(std::expected<Snapshot, DecodingError>{snapshot}));

    EXPECT_CALL(orderbook, initialize(Field(&Snapshot::last_update_id, kLastUpdateId))).Times(1);
    EXPECT_CALL(orderbook, apply(update_matcher)).Times(1);
    EXPECT_CALL(sequencer, initialize(kLastUpdateId)).Times(1);
    EXPECT_CALL(sequencer, check(update_matcher)).Times(1);

    binance::RecoveryManager recovery_manager(gateway, decoder, sequencer, orderbook);

    ASSERT_TRUE(recovery_manager.begin_initialize().has_value());

    recovery_manager.buffer_update(update);

    const auto recover_res = recovery_manager.try_recover();

    ASSERT_TRUE(recover_res.has_value());
    EXPECT_FALSE(recovery_manager.is_recovering());
}

TEST(RecoveryManagerTest, TryRecoverSkipsStaleUpdate)
{
    MockGateway gateway;
    MockDecoder decoder;
    MockSequencer sequencer;
    MockOrderbook orderbook;
    Snapshot snapshot = make_snapshot();
    SequencedBookUpdate stale_update = make_update(90);
    SequencedBookUpdate applicable_update = make_update();
    const auto stale_matcher = AllOf(
        Field(&SequencedBookUpdate::symbol, Symbol::BTCUSDT),
        Field(&SequencedBookUpdate::first_update, stale_update.first_update),
        Field(&SequencedBookUpdate::last_update, stale_update.last_update));
    const auto applicable_matcher = AllOf(
        Field(&SequencedBookUpdate::symbol, Symbol::BTCUSDT),
        Field(&SequencedBookUpdate::first_update, applicable_update.first_update),
        Field(&SequencedBookUpdate::last_update, applicable_update.last_update));

    EXPECT_CALL(orderbook, reset()).Times(2);
    EXPECT_CALL(sequencer, reset()).Times(2);
    EXPECT_CALL(gateway, start()).Times(1);
    EXPECT_CALL(gateway, wait_until_running(_))
        .WillOnce(Return(std::expected<void, GatewayError>{}));

    EXPECT_CALL(gateway, request_snapshot())
        .Times(1)
        .WillOnce(Return(std::expected<std::string, GatewayError>{kValidBinanceSnapshot}));

    EXPECT_CALL(decoder, decode_snapshot(std::string_view{kValidBinanceSnapshot}))
        .Times(1)
        .WillOnce(Return(std::expected<Snapshot, DecodingError>{snapshot}));

    EXPECT_CALL(orderbook, initialize(Field(&Snapshot::last_update_id, kLastUpdateId))).Times(1);
    EXPECT_CALL(sequencer, initialize(kLastUpdateId)).Times(1);
    EXPECT_CALL(sequencer, check(stale_matcher)).Times(0);
    EXPECT_CALL(orderbook, apply(stale_matcher)).Times(0);
    EXPECT_CALL(sequencer, check(applicable_matcher)).Times(1)
        .WillOnce(Return(std::expected<void, SequencingError>{}));
    EXPECT_CALL(orderbook, apply(applicable_matcher)).Times(1);

    binance::RecoveryManager recovery_manager(gateway, decoder, sequencer, orderbook);

    ASSERT_TRUE(recovery_manager.begin_initialize().has_value());

    recovery_manager.buffer_update(stale_update);
    recovery_manager.buffer_update(applicable_update);

    const auto recover_res = recovery_manager.try_recover();

    ASSERT_TRUE(recover_res.has_value());
    EXPECT_FALSE(recovery_manager.is_recovering());
}

TEST(RecoveryManagerTest, TryRecoverReinitializes)
{
    MockGateway gateway;
    MockDecoder decoder;
    MockSequencer sequencer;
    MockOrderbook orderbook;
    Snapshot snapshot = make_snapshot();
    SequencedBookUpdate first_update = make_update();
    SequencedBookUpdate gap_update = make_update(104);
    const auto first_matcher = AllOf(
        Field(&SequencedBookUpdate::symbol, Symbol::BTCUSDT),
        Field(&SequencedBookUpdate::first_update, first_update.first_update),
        Field(&SequencedBookUpdate::last_update, first_update.last_update));
    const auto gap_matcher = AllOf(
        Field(&SequencedBookUpdate::symbol, Symbol::BTCUSDT),
        Field(&SequencedBookUpdate::first_update, gap_update.first_update),
        Field(&SequencedBookUpdate::last_update, gap_update.last_update));

    EXPECT_CALL(orderbook, reset()).Times(3);
    EXPECT_CALL(sequencer, reset()).Times(3);
    EXPECT_CALL(gateway, start()).Times(2);
    EXPECT_CALL(gateway, wait_until_running(_)).Times(2)
        .WillRepeatedly(Return(std::expected<void, GatewayError>{}));

    EXPECT_CALL(gateway, request_snapshot())
        .Times(1)
        .WillOnce(Return(std::expected<std::string, GatewayError>{kValidBinanceSnapshot}));

    EXPECT_CALL(decoder, decode_snapshot(std::string_view{kValidBinanceSnapshot}))
        .Times(1)
        .WillOnce(Return(std::expected<Snapshot, DecodingError>{snapshot}));

    EXPECT_CALL(orderbook, initialize(Field(&Snapshot::last_update_id, kLastUpdateId))).Times(1);
    EXPECT_CALL(sequencer, initialize(kLastUpdateId)).Times(1);
    EXPECT_CALL(sequencer, check(first_matcher)).Times(1)
        .WillOnce(Return(std::expected<void, SequencingError>{}));
    EXPECT_CALL(orderbook, apply(first_matcher)).Times(1);
    EXPECT_CALL(sequencer, check(gap_matcher)).Times(1)
        .WillOnce(Return(std::unexpected(SequencingError::GAP_DETECTED)));
    EXPECT_CALL(orderbook, apply(gap_matcher)).Times(0);

    binance::RecoveryManager recovery_manager(gateway, decoder, sequencer, orderbook);

    ASSERT_TRUE(recovery_manager.begin_initialize().has_value());

    recovery_manager.buffer_update(first_update);
    recovery_manager.buffer_update(gap_update);

    const auto recover_res = recovery_manager.try_recover();

    ASSERT_TRUE(recover_res.has_value());
    EXPECT_TRUE(recovery_manager.is_recovering());
}

TEST(RecoveryManagerTest, StopClearsState)
{
    MockGateway gateway;
    MockDecoder decoder;
    MockSequencer sequencer;
    MockOrderbook orderbook;

    EXPECT_CALL(orderbook, reset()).Times(2);
    EXPECT_CALL(sequencer, reset()).Times(2);
    EXPECT_CALL(gateway, start()).Times(1);
    EXPECT_CALL(gateway, wait_until_running(_))
        .WillOnce(Return(std::expected<void, GatewayError>{}));
    EXPECT_CALL(gateway, stop()).Times(1);
    EXPECT_CALL(gateway, request_snapshot()).Times(0);
    EXPECT_CALL(decoder, decode_snapshot(_)).Times(0);
    EXPECT_CALL(orderbook, initialize(_)).Times(0);
    EXPECT_CALL(orderbook, apply(_)).Times(0);
    EXPECT_CALL(sequencer, initialize(_)).Times(0);
    EXPECT_CALL(sequencer, check(_)).Times(0);

    binance::RecoveryManager recovery_manager(gateway, decoder, sequencer, orderbook);

    ASSERT_TRUE(recovery_manager.begin_initialize().has_value());

    recovery_manager.buffer_update(make_update());
    recovery_manager.stop();

    const auto recover_res = recovery_manager.try_recover();

    ASSERT_TRUE(recover_res.has_value());
    EXPECT_FALSE(recovery_manager.is_recovering());
    EXPECT_EQ(recovery_manager.state(), IRecoveryManager::State::STOPPED);
}

} // namespace
