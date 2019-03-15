#include <gtest/gtest.h>
#include <thread>

std::thread gCallbackThread;

int
main(int argc, char* argv[])
{
    ::testing::InitGoogleTest(&argc, argv);
    int res = RUN_ALL_TESTS();
    return res;
}
