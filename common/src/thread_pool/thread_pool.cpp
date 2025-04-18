/* Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * SPDX-License-Identifier: MulanPSL-2.0
 */

#include "thread_pool/thread_pool.h"

ThreadPool::ThreadPool(uint32_t threadNum, uint64_t maxTaskNum, std::string name)
    : threadNum(threadNum),
      maxTaskNum(maxTaskNum),
      name(std::move(name)),
      taskSem(maxTaskNum)
{
}

ThreadPool::~ThreadPool() { Stop(); }

std::unique_ptr<ThreadPool>
ThreadPool::CreateThreadPool(uint32_t threadNum, uint64_t maxTaskNum, const std::string &name)
{
    return std::make_unique<ThreadPool>(threadNum, maxTaskNum, name);
}

int ThreadPool::Start()
{
    std::jthread t;
    std::string threadName;
    try {
        for (uint32_t i = 0;i < threadNum; ++i) {
            threadName = name + "_" + std::to_string(i);
            t = std::jthread([this, threadName]() { WorkLoop(threadName); });
            pthread_setname_np(t.native_handle(), threadName.c_str());
            threads.emplace_back(std::move(t));
        }
    } catch (const std::exception &e) {
        Stop();
        return 1;
    }

    return 0;
}

void ThreadPool::Stop()
{
    stopSource.request_stop();
    cv.notify_all();
    threads.clear();
}

int ThreadPool::Submit(const ThreadTask &func)
{
    taskSem.acquire();
    {
        std::lock_guard lock(mutex);
        tasks.push(func);
    }

    cv.notify_one();
    return 0;
}

void ThreadPool::WorkLoop(const std::string &)
{
    auto token = stopSource.get_token();
    std::unique_lock lock(mutex);
    while (!token.stop_requested() || !tasks.empty()) {
        cv.wait(lock, token, [this, token] { return !tasks.empty() || token.stop_requested(); });
        if (tasks.empty()) {
            break;
        }

        auto task = std::move(tasks.front());
        tasks.pop();
        lock.unlock();

        if (task.task) {
            task.task();
        }
        taskSem.release();
        lock.lock();
    }
}
