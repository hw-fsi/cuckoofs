/* Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * SPDX-License-Identifier: MulanPSL-2.0
 */

#pragma once

#include <stdint.h>
#include <unistd.h>
#include <cstdlib>
#include <expected>
#include <optional>
#include <string>

#include <sys/stat.h>

#define TEST_RDONLY 0x8000
#define OPENMODE_CHECK 0x0FFF
#define RPC_BYTES_ADDITION (sizeof(int) + sizeof(char))
#define GRPC_DEFAULT_SEND_MESSAGE_SIZE 4 * 1024 * 1024

extern uint32_t CUCKOO_BLOCK_SIZE;
extern uint32_t READ_BIGFILE_SIZE;

struct StatFSBuf
{
    uint64_t f_bsize;
    uint64_t f_frsize;
    uint64_t f_blocks;
    uint64_t f_bfree;
    uint64_t f_bavail;
    uint64_t f_files;
    uint64_t f_ffree;
    uint64_t f_favail;
    uint64_t f_fsid;
    uint64_t f_flag;
    uint64_t f_namemax;
};

void SetRootPath(std::string str);
void SetTotalDirectory(int num);
std::string GetFilePath(uint64_t inodeId);
int GenerateRandom(int minValue, int maxValue);
std::string GetOBSFilePath(std::string path);
std::optional<std::string> GetUserName();
std::optional<std::string_view> SplitIp(std::string_view ipPort);
std::string GetUFSPath(std::string path);
std::expected<std::string, std::string> GetPodIPPort();
bool TestOBS();
float GetBackGroundEvictRatio();
float GetStorageThreshold();
int GetParentPathLevel();
