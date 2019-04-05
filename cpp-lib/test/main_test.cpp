#include "spdlog/spdlog.h"

#include <gtest/gtest.h>
#include <thread>

std::thread gCallbackThread;

int
main(int argc, char* argv[])
{
    spdlog::set_level(spdlog::level::off);
    ::testing::InitGoogleTest(&argc, argv);
    int res = RUN_ALL_TESTS();
    return res;
}
