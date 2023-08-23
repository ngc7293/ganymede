#include <cstdlib>

#include <gtest/gtest.h>

#include <ganymede/log/log.hh>

int main(int argc, const char* argv[])
{
    setenv("TZ", "UTC", 1);

    ganymede::log::Log::get().AddSink(std::make_unique<ganymede::log::StdIoLogSink>());

    testing::InitGoogleTest(&argc, (char**)argv);
    return RUN_ALL_TESTS();
}