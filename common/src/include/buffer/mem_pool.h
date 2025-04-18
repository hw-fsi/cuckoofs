/* Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * SPDX-License-Identifier: MulanPSL-2.0
 */

#pragma once

#include <pthread.h>
#include <atomic>
#include <cstdlib>
#include <queue>
#include <vector>

class MemPool {
  public:
    static MemPool &GetInstance()
    {
        static MemPool instance;
        return instance;
    }

    MemPool() = default;

    MemPool(size_t blockSize, size_t capacity)
    {
        if (!m_init.exchange(true)) {
            m_blockSize = blockSize;
            m_capacity = capacity;
            m_memLock = new (std::nothrow) pthread_spinlock_t;
            if (m_memLock != nullptr) {
                pthread_spin_init(m_memLock, 0);
            } else {
                m_init.store(false);
            }
        }
    }

    ~MemPool()
    {
        while (!m_freeBlocks.empty()) {
            free(m_freeBlocks.front());
            m_freeBlocks.pop();
        }
        if (m_memLock) {
            delete m_memLock;
            m_memLock = nullptr;
        }
    }

    void init(size_t blockSize, size_t capacity)
    {
        if (!m_init.exchange(true)) {
            m_blockSize = blockSize;
            m_capacity = capacity;
            m_memLock = new (std::nothrow) pthread_spinlock_t;
            if (m_memLock != nullptr) {
                pthread_spin_init(m_memLock, 0);
            } else {
                m_init.store(false);
            }
        }
    }

    void *alloc()
    {
        if (!m_init.load()) {
            return nullptr;
        }
        void *block = nullptr;
        pthread_spin_lock(m_memLock);
        if (!m_freeBlocks.empty()) {
            block = m_freeBlocks.front();
            m_freeBlocks.pop();
        }
        pthread_spin_unlock(m_memLock);
        block = block ? block : aligned_alloc(512, m_blockSize);
        m_size += block ? 1 : 0;
        return block;
    }

    std::vector<void *> calloc(int num)
    {
        if (!m_init.load()) {
            return {};
        }
        std::vector<void *> bulkMem;

        pthread_spin_lock(m_memLock);
        while (!m_freeBlocks.empty() && num--) {
            bulkMem.emplace_back(m_freeBlocks.front());
            m_freeBlocks.pop();
        }
        pthread_spin_unlock(m_memLock);
        while (num > 0) {
            void *mem = aligned_alloc(512, m_blockSize);
            if (mem == nullptr) {
                break;
            }
            bulkMem.emplace_back(mem);
            num--;
        }

        if (num > 0) {
            /* error */
            for (auto &m : bulkMem) {
                /* how to free? */
                ::free(m);
            }
            bulkMem.clear();
        }
        m_size += bulkMem.size();
        return bulkMem;
    }

    void free(void *buf)
    {
        if (!m_init.load()) {
            return;
        }
        if (buf == nullptr) {
            return;
        }
        if (m_size-- < m_capacity) {
            pthread_spin_lock(m_memLock);
            m_freeBlocks.push(buf);
            pthread_spin_unlock(m_memLock);
        } else {
            ::free(buf);
        }
    }

  private:
    std::atomic<bool> m_init = false;
    size_t m_blockSize = 0;
    size_t m_capacity = 0;
    std::atomic<size_t> m_size = 0;
    std::queue<void *> m_freeBlocks;
    pthread_spinlock_t *m_memLock = nullptr;
};
