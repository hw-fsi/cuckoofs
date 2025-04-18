/* Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * SPDX-License-Identifier: MulanPSL-2.0
 */

#include "stats/cuckoo_stats.h"

#include <condition_variable>
#include <csignal>
#include <fstream>

#include "log/logging.h"

std::string formatU64(size_t size)
{
    if (size == 0)
        return "0";

    constexpr std::array units = {"B", "KiB", "MiB", "GiB", "TiB", "PiB"};
    int unitIdx = 0;

    while (size >= 10000 && unitIdx < std::ssize(units) - 1) {
        size >>= 10;
        unitIdx++;
    }

    return std::format("{}{}", size, units[unitIdx]);
}

std::string formatOp(size_t size)
{
    if (size == 0)
        return "0";

    constexpr std::array units = {"", "Ki", "Mi", "Gi"};
    int unitIdx = 0;

    while (size >= 10000 && unitIdx < std::ssize(units) - 1) {
        size >>= 10;
        unitIdx++;
    }

    return std::format("{}{}", size, units[unitIdx]);
}

double formatTime(size_t mus, size_t ops)
{
    if (ops == 0)
        return static_cast<double>(mus);

    double time = static_cast<double>(mus) / (ops * 1000);
    const auto precision = time < 10 ? 3 : time < 100 ? 2 : time < 1000 ? 1 : 0;

    time = std::round(time * std::pow(10, precision)) / std::pow(10, precision);
    return time < 0.001 ? 0 : time;
}

void PrintStats(std::string_view mountPath, std::stop_token stoken)
{
    std::mutex mtx;
    std::condition_variable cv;
    auto nextWake = std::chrono::steady_clock::now() + std::chrono::seconds(1);
    // Process mount path
    auto lastNonSlash = mountPath.find_last_not_of('/');
    auto lastSlash = mountPath.substr(0, lastNonSlash).find_last_of('/');
    std::string statPath = std::format("{}/stats.out", mountPath.substr(0, lastSlash + 1));

    std::remove(statPath.c_str());
    std::ofstream outFile;
    size_t currentStats[STATS_END];
    errno_t err = memset_s(currentStats, sizeof(size_t) * STATS_END, 0, sizeof(size_t) * STATS_END);
    if (err != 0) {
        CUCKOO_LOG(LOG_ERROR) << "Secure func failed: " << err;
        return;
    }

    while (!stoken.stop_requested()) {
        for (int i = 0; i < STATS_END; i++) {
            currentStats[i] = CuckooStats::GetInstance().stats[i].exchange(currentStats[i]);
        }
        outFile.open(statPath, std::ios::out);
        if (!outFile.is_open()) {
            CUCKOO_LOG(LOG_ERROR) << "Open stats file " << statPath << " failed: " << strerror(errno);
            std::unique_lock lock(mtx);
            cv.wait_for(lock, std::chrono::seconds(5), [&] { return stoken.stop_requested(); });
            continue;
        }

        // Using std::println for all outputs
        std::println(outFile, "Cuckoo File System Statistics");
        std::println(outFile, "============================");
        std::println(outFile);

        // FUSE Operations
        std::println(outFile, "FUSE Operations:");
        std::println(outFile, "  Total Operations: {}", currentStats[FUSE_OPS]);
        std::println(outFile, "  Average Latency: {} μs", formatTime(currentStats[FUSE_LAT], currentStats[FUSE_OPS]));
        std::println(outFile, "  Read: {}", formatU64(currentStats[FUSE_READ]));
        std::println(outFile, "  Read Operations: {}", currentStats[FUSE_READ_OPS]);
        std::println(outFile,
                     "  Read Average Latency: {} μs",
                     formatTime(currentStats[FUSE_READ_LAT], currentStats[FUSE_READ_OPS]));
        std::println(outFile, "  Write: {}", formatU64(currentStats[FUSE_WRITE]));
        std::println(outFile, "  Write Operations: {}", currentStats[FUSE_WRITE_OPS]);
        std::println(outFile,
                     "  Write Average Latency: {} μs\n",
                     formatTime(currentStats[FUSE_WRITE_LAT], currentStats[FUSE_WRITE_OPS]));

        // Metadata Operations
        std::println(outFile, "Metadata Operations:");
        // std::println(outFile, "  Total Operations: {}", formatOp(currentStats[META_OPS]));
        // std::println(outFile, "  Average Latency: {} μs", formatTime(currentStats[META_LAT],

        std::println(outFile, "  Open: {}", currentStats[META_OPEN]);
        std::println(outFile, "  Open Atomic: {}", currentStats[META_OPEN_ATOMIC]);
        std::println(outFile, "  Release: {}", currentStats[META_RELEASE]);
        std::println(outFile, "  Stat: {}", currentStats[META_STAT]);
        std::println(outFile,
                     "  Stat Latency: {} μs",
                     formatTime(currentStats[META_STAT_LAT], currentStats[META_STAT]));
        std::println(outFile, "  Lookup: {}", currentStats[META_LOOKUP]);
        std::println(outFile, "  Create: {}", currentStats[META_CREATE]);
        std::println(outFile, "  Unlink: {}", currentStats[META_UNLINK]);
        std::println(outFile, "  Mkdir: {}", currentStats[META_MKDIR]);
        std::println(outFile, "  Rmdir: {}", currentStats[META_RMDIR]);
        std::println(outFile, "  Opendir: {}", currentStats[META_OPENDIR]);
        std::println(outFile, "  Readdir: {}", currentStats[META_READDIR]);
        std::println(outFile, "  Rename: {}", currentStats[META_RENAME]);
        std::println(outFile, "  Access: {}", currentStats[META_ACCESS]);
        std::println(outFile, "  Releasedir: {}", currentStats[META_RELEASEDIR]);
        std::println(outFile, "  Truncate: {}", currentStats[META_TRUNCATE]);
        std::println(outFile, "  Flush: {}", currentStats[META_FLUSH]);
        std::println(outFile, "  Fsync: {}", currentStats[META_FSYNC]);

        // Block Cache and Object Operations
        std::println(outFile, "\nBlock Cache Operations:");
        std::println(outFile, "  Reads: {}", formatU64(currentStats[BLOCKCACHE_READ]));
        std::println(outFile, "  Writes: {}", formatU64(currentStats[BLOCKCACHE_WRITE]));

        std::println(outFile, "\nObject Operations:");
        std::println(outFile, "  Gets: {}", currentStats[OBJ_GET]);
        std::println(outFile, "  Puts: {}", currentStats[OBJ_PUT]);

        outFile.close();
        {
            std::unique_lock lock(mtx);
            cv.wait_until(lock, nextWake, [&] { return stoken.stop_requested(); });
        }
        nextWake += std::chrono::seconds(1);
    }
    CUCKOO_LOG(LOG_INFO) << "Stats collection terminated gracefully";
}
