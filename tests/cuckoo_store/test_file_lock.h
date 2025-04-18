#pragma once

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "util/file_lock.h"

class FileLockUT : public testing::TestWithParam<std::tuple<LockMode, LockMode, bool>> {
  public:
    static void SetUpTestSuite() {}
    static void TearDownTestSuite() {}
    void SetUp() override {}
    void TearDown() override {}

    static FileLock flk;
    static uint64_t id;
};
