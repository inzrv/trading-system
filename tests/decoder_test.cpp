#include <gtest/gtest.h>

#include "binance/decoder.h"

namespace
{

const std::string kValidDiffPayload = R"({
    "e":"depthUpdate",
    "E":123456789,
    "s":"BTCUSDT",
    "U":101,
    "u":102,
    "b":[["100.10","1.25"]],
    "a":[["100.20","2.50"]]
})";

const std::string kValidSnapshotPayload = R"({
    "lastUpdateId":100,
    "bids":[["100.10","1.25"]],
    "asks":[["100.20","2.50"]]
})";

TEST(DecoderTest, DecodeDiff)
{
    const binance::Decoder decoder;

    const auto res = decoder.decode_diff(kValidDiffPayload);

    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->symbol, Symbol::BTCUSDT);
    EXPECT_EQ(res->first_update, 101);
    EXPECT_EQ(res->last_update, 102);
    ASSERT_EQ(res->bids.size(), 1u);
    EXPECT_DOUBLE_EQ(res->bids[0].price, 100.10);
    EXPECT_DOUBLE_EQ(res->bids[0].quantity, 1.25);
    ASSERT_EQ(res->asks.size(), 1u);
    EXPECT_DOUBLE_EQ(res->asks[0].price, 100.20);
    EXPECT_DOUBLE_EQ(res->asks[0].quantity, 2.50);
}

TEST(DecoderTest, DecodeDiffParseFails)
{
    const binance::Decoder decoder;

    const auto res = decoder.decode_diff("{");

    ASSERT_FALSE(res.has_value());
    EXPECT_EQ(res.error(), DecodingError::PAYLOAD_PARSING_ERROR);
}

TEST(DecoderTest, DecodeDiffUnexpectedEvent)
{
    const binance::Decoder decoder;
    const std::string payload = R"({
        "e":"trade",
        "E":123456789,
        "s":"BTCUSDT",
        "U":101,
        "u":102,
        "b":[["100.10","1.25"]],
        "a":[["100.20","2.50"]]
    })";

    const auto res = decoder.decode_diff(payload);

    ASSERT_FALSE(res.has_value());
    EXPECT_EQ(res.error(), DecodingError::UNEXPECTED_VALUE);
}

TEST(DecoderTest, DecodeDiffMissingField)
{
    const binance::Decoder decoder;
    const std::string payload = R"({
        "e":"depthUpdate",
        "E":123456789,
        "s":"BTCUSDT",
        "U":101,
        "b":[["100.10","1.25"]],
        "a":[["100.20","2.50"]]
    })";

    const auto res = decoder.decode_diff(payload);

    ASSERT_FALSE(res.has_value());
    EXPECT_EQ(res.error(), DecodingError::INVALID_PAYLOAD);
}

TEST(DecoderTest, DecodeDiffInvalidLevel)
{
    const binance::Decoder decoder;
    const std::string payload = R"({
        "e":"depthUpdate",
        "E":123456789,
        "s":"BTCUSDT",
        "U":101,
        "u":102,
        "b":[["100.10"]],
        "a":[["100.20","2.50"]]
    })";

    const auto res = decoder.decode_diff(payload);

    ASSERT_FALSE(res.has_value());
    EXPECT_EQ(res.error(), DecodingError::INVALID_PAYLOAD);
}

TEST(DecoderTest, DecodeDiffInvalidUpdateRange)
{
    const binance::Decoder decoder;
    const std::string payload = R"({
        "e":"depthUpdate",
        "E":123456789,
        "s":"BTCUSDT",
        "U":102,
        "u":101,
        "b":[["100.10","1.25"]],
        "a":[["100.20","2.50"]]
    })";

    const auto res = decoder.decode_diff(payload);

    ASSERT_FALSE(res.has_value());
    EXPECT_EQ(res.error(), DecodingError::INVALID_PAYLOAD);
}

TEST(DecoderTest, DecodeSnapshot)
{
    const binance::Decoder decoder;

    const auto res = decoder.decode_snapshot(kValidSnapshotPayload);

    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->last_update_id, 100);
    ASSERT_EQ(res->bids.size(), 1u);
    EXPECT_DOUBLE_EQ(res->bids[0].price, 100.10);
    EXPECT_DOUBLE_EQ(res->bids[0].quantity, 1.25);
    ASSERT_EQ(res->asks.size(), 1u);
    EXPECT_DOUBLE_EQ(res->asks[0].price, 100.20);
    EXPECT_DOUBLE_EQ(res->asks[0].quantity, 2.50);
}

TEST(DecoderTest, DecodeSnapshotParseFails)
{
    const binance::Decoder decoder;

    const auto res = decoder.decode_snapshot("{");

    ASSERT_FALSE(res.has_value());
    EXPECT_EQ(res.error(), DecodingError::PAYLOAD_PARSING_ERROR);
}

TEST(DecoderTest, DecodeSnapshotInvalidLevel)
{
    const binance::Decoder decoder;
    const std::string payload = R"({
        "lastUpdateId":100,
        "bids":[["100.10","oops"]],
        "asks":[["100.20","2.50"]]
    })";

    const auto res = decoder.decode_snapshot(payload);

    ASSERT_FALSE(res.has_value());
    EXPECT_EQ(res.error(), DecodingError::INVALID_PAYLOAD);
}

} // namespace
