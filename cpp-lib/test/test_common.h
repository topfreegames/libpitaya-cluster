#ifndef PITAYA_TEST_COMMON_H
#define PITAYA_TEST_COMMON_H

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <thread>

extern std::thread gCallbackThread;

ACTION_TEMPLATE(ExecuteCallback, HAS_1_TEMPLATE_PARAMS(unsigned, Index), AND_2_VALUE_PARAMS(a, b))
{
    auto fn = std::get<Index>(args);

    if (gCallbackThread.joinable()) {
        gCallbackThread.join();
    }

    gCallbackThread = std::thread([=] { fn(a, b); });
}

#endif // PITAYA_TEST_COMMON_H
