/* Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * SPDX-License-Identifier: MulanPSL-2.0
 */

#pragma once

#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <semaphore>
#include <thread>
#include <vector>

struct ThreadTask
{
    std::string taskName;
    std::function<void()> task;
};

class ThreadPool {
  public:
    ThreadPool(uint32_t threadNum, uint64_t maxTaskNum, std::string name);

    ~ThreadPool();

    static std::unique_ptr<ThreadPool>
    CreateThreadPool(uint32_t threadNum, uint64_t maxTaskNum, const std::string &name);

    int Start();

    void Stop();

    int Submit(const ThreadTask &func);

  private:
    void WorkLoop(const std::string &threadName);

    uint32_t threadNum{};
    uint64_t maxTaskNum{};
    std::string name;
    std::mutex mutex;
    std::condition_variable_any cv;
    std::queue<ThreadTask> tasks;
    std::vector<std::jthread> threads;
    std::stop_source stopSource;
    std::counting_semaphore<> taskSem;
};
