#include <gtest/gtest.h>

#include "test_helpers.h"
#include "binance/sequencer.h"

namespace
{

TEST(SequencerTest, CheckFailsWithoutInitialize)
{
    binance::Sequencer sequencer;

    const auto res = sequencer.check(make_update(101, 101));

    ASSERT_FALSE(res.has_value());
    EXPECT_EQ(res.error(), SequencingError::NOT_INITIALIZED);
}

TEST(SequencerTest, CheckSucceedsAfterInitialize)
{
    binance::Sequencer sequencer;
    sequencer.initialize(100);

    const auto res = sequencer.check(make_update(101, 101));

    ASSERT_TRUE(res.has_value());
}

TEST(SequencerTest, CheckIgnoresOldUpdate)
{
    binance::Sequencer sequencer;
    sequencer.initialize(100);

    const auto res = sequencer.check(make_update(99, 100));

    ASSERT_TRUE(res.has_value());
}

TEST(SequencerTest, CheckDetectsGap)
{
    binance::Sequencer sequencer;
    sequencer.initialize(100);

    const auto res = sequencer.check(make_update(103, 103));

    ASSERT_FALSE(res.has_value());
    EXPECT_EQ(res.error(), SequencingError::GAP_DETECTED);
}

TEST(SequencerTest, CheckAcceptsBridgingUpdate)
{
    binance::Sequencer sequencer;
    sequencer.initialize(100);

    const auto res = sequencer.check(make_update(100, 101));

    ASSERT_TRUE(res.has_value());
}

TEST(SequencerTest, CheckUpdatesSequenceState)
{
    binance::Sequencer sequencer;
    sequencer.initialize(100);

    ASSERT_TRUE(sequencer.check(make_update(101, 102)).has_value());

    const auto res = sequencer.check(make_update(103, 104));

    ASSERT_TRUE(res.has_value());
}

TEST(SequencerTest, ResetClearsInitialization)
{
    binance::Sequencer sequencer;
    sequencer.initialize(100);
    sequencer.reset();

    const auto res = sequencer.check(make_update(101, 101));

    ASSERT_FALSE(res.has_value());
    EXPECT_EQ(res.error(), SequencingError::NOT_INITIALIZED);
}

} // namespace
