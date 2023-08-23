#include <cstdlib>

#include <gtest/gtest.h>

int main(int argc, const char* argv[])
{
    setenv("TZ", "UTC", 1);

    testing::InitGoogleTest(&argc, (char**)argv);
    return RUN_ALL_TESTS();
}