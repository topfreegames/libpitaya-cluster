#include "test_common.h"

#include "pitaya/utils/ticker.h"

#ifdef _WIN32
#include <windows.h>
#pragma comment(lib, "winmm.lib") // for timeBeginPeriod
#endif

using namespace ::testing;
using namespace pitaya::utils;

TEST(Ticker, DestructorStopsTheTicker)
{
    bool called = false;
    Ticker ticker(std::chrono::milliseconds(50), [&]() { called = true; });

    ticker.Start();
}

TEST(Ticker, CallbackIsNotCalledIfNotStarted)
{
    bool called = false;
    Ticker ticker(std::chrono::milliseconds(50), [&]() { called = true; });

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    ASSERT_FALSE(called);
}

TEST(Ticker, CallbackIsCalledIfStarted)
{
#ifdef _WIN32
    UINT resolutionSetResult = timeBeginPeriod(5);
    if (resolutionSetResult == TIMERR_NOCANDO) {
        GTEST_SKIP() << "Windows can't ensure time resolution required.";
    }
#endif

    bool called = false;
    Ticker ticker(std::chrono::milliseconds(10), [&]() { called = true; });

    ticker.Start();
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    ticker.Stop();

#ifdef _WIN32
    timeEndPeriod(5);
#endif

    ASSERT_TRUE(called);
}

TEST(Ticker, CallbackIsNotCalledIfStartedAndStopped)
{
#ifdef _WIN32
    UINT resolutionSetResult = timeBeginPeriod(5);
    if(resolutionSetResult == TIMERR_NOCANDO){
        GTEST_SKIP() << "Windows can't ensure time resolution required.";
    }
#endif

    bool called = false;
    Ticker ticker(std::chrono::milliseconds(10), [&]() { called = true; });
    
    ticker.Start();
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    ticker.Stop();

#ifdef _WIN32
    timeEndPeriod(5);
#endif

    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    ASSERT_FALSE(called);
}

TEST(Ticker, CallbackIsCalledNTimesWithLimitedPrecision)
{
    size_t called = 0;
    Ticker ticker(std::chrono::milliseconds(100), [&]() { ++called; });

    ticker.Start();
    std::this_thread::sleep_for(std::chrono::milliseconds(1100));
    ticker.Stop();

    EXPECT_EQ(called, 10);
}

TEST(Ticker, DoesNotRespectTheIntervalForReallySmallOnes)
{
    size_t called = 0;
    Ticker ticker(std::chrono::milliseconds(1), [&]() { ++called; });

    ticker.Start();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    ticker.Stop();

    EXPECT_LT(called, 95);
}

// The test below is not valid on Windows, as its launch::async policy is
// implemented as launch::async|launch::deferred, not guaranteeing that the
// callback will be executed in a different thread.
#ifndef _WIN32
TEST(Ticker, CallbackIsNotCalledFromTheMainThread)
{
    auto callbackThreadId = std::this_thread::get_id();

    Ticker ticker(std::chrono::milliseconds(1),
                  [&]() { callbackThreadId = std::this_thread::get_id(); });

    ticker.Start();
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    ticker.Stop();

    EXPECT_NE(callbackThreadId, std::this_thread::get_id());
}
#endif
