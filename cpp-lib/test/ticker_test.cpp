#include "test_common.h"

#include "pitaya/utils/ticker.h"

using namespace ::testing;
using namespace pitaya::utils;

TEST(Ticker, CallbackIsNotCalledIfNotStarted)
{
    bool called = false;
    Ticker ticker(std::chrono::milliseconds(50), [&]() {
        called = true;
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    ASSERT_FALSE(called);
}

TEST(Ticker, CallbackIsCalledIfStarted)
{
    bool called = false;
    Ticker ticker(std::chrono::milliseconds(10), [&]() {
        called = true;
    });

    ticker.Start();

    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    ASSERT_TRUE(called);
}

TEST(Ticker, CallbackIsNotCalledIfStartedAndStopped)
{
    bool called = false;
    Ticker ticker(std::chrono::milliseconds(10), [&]() {
        called = true;
    });

    ticker.Start();
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    ticker.Stop();

    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    ASSERT_FALSE(called);
}

TEST(Ticker, CallbackIsCalledNTimesWithLimitedPrecision)
{
    size_t called = 0;
    Ticker ticker(std::chrono::milliseconds(100), [&]() {
        ++called;
    });

    ticker.Start();

    std::this_thread::sleep_for(std::chrono::milliseconds(1050));

    EXPECT_EQ(called, 10);
}

TEST(Ticker, DoesNotRespectTheIntervalForReallySmallOnes)
{
    size_t called = 0;
    Ticker ticker(std::chrono::milliseconds(1), [&]() {
        ++called;
    });

    ticker.Start();

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    EXPECT_LT(called, 90);
}
