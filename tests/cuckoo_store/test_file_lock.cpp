#include "test_file_lock.h"

#include <future>

FileLock FileLockUT::flk;
uint64_t FileLockUT::id = 0;

TEST_P(FileLockUT, TryLock)
{
    flk.TryGetFileLock(id, std::get<0>(GetParam()));
    bool succ = flk.TryGetFileLock(id, std::get<1>(GetParam()));
    EXPECT_EQ(succ, std::get<2>(GetParam()));
    flk.ReleaseFileLock(id, std::get<0>(GetParam()));
    if (succ) {
        flk.ReleaseFileLock(id, std::get<1>(GetParam()));
    }
}

TEST_P(FileLockUT, WaitLock)
{
    flk.WaitGetFileLock(id, std::get<0>(GetParam()));
    auto fut1 = std::async(std::launch::async, [&]() { flk.WaitGetFileLock(id, std::get<1>(GetParam())); });
    auto status = fut1.wait_for(std::chrono::milliseconds(100));

    EXPECT_EQ(status == std::future_status::ready, std::get<2>(GetParam()));
    flk.ReleaseFileLock(id, std::get<0>(GetParam()));
    fut1.wait();
    flk.ReleaseFileLock(id, std::get<1>(GetParam()));
}

TEST_F(FileLockUT, TestLocked)
{
    bool succ = flk.TestLocked(id, LockMode::S);
    EXPECT_EQ(succ, false);
    succ = flk.TestLocked(id, LockMode::X);
    EXPECT_EQ(succ, false);

    flk.TryGetFileLock(id, LockMode::S);
    succ = flk.TestLocked(id, LockMode::S);
    EXPECT_EQ(succ, false);
    succ = flk.TestLocked(id, LockMode::X);
    EXPECT_EQ(succ, true);
    flk.ReleaseFileLock(id, LockMode::S);

    flk.TryGetFileLock(id, LockMode::X);
    succ = flk.TestLocked(id, LockMode::S);
    EXPECT_EQ(succ, true);
    succ = flk.TestLocked(id, LockMode::X);
    EXPECT_EQ(succ, true);
    flk.ReleaseFileLock(id, LockMode::X);
}

INSTANTIATE_TEST_SUITE_P(FileLockSuite,
                         FileLockUT,
                         ::testing::Values(std::make_tuple(LockMode::S, LockMode::S, true),
                                           std::make_tuple(LockMode::S, LockMode::X, false),
                                           std::make_tuple(LockMode::X, LockMode::S, false),
                                           std::make_tuple(LockMode::X, LockMode::X, false)));

int main(int argc, char **argv)
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
