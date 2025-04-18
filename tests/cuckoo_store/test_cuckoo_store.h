#pragma once

#include <cerrno>
#include <filesystem>
#include <sstream>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "brpc/brpc_server.h"
#include "conf/cuckoo_property_key.h"
#include "cuckoo_store/cuckoo_store.h"
#include "init/cuckoo_init.h"
#include "remote_connection_utils/error_code_def.h"

class CuckooStoreUT : public testing::Test {
  public:
    static void SetUpTestSuite()
    {
        std::cout << "Calling SetUpTestSuite!" << std::endl;
        int ret = GetInit().Init();
        if (ret != 0) {
            exit(1);
        }
        config = GetInit().GetCuckooConfig();
        try {
            auto cachePath = config->GetString(CuckooPropertyKey::CUCKOO_CACHE_ROOT);
            if (std::filesystem::exists(cachePath)) {
                std::filesystem::remove_all(cachePath);
                std::cout << "已删除目录及其内容: " << cachePath << std::endl;
            }
            std::filesystem::create_directory(cachePath);
            std::cout << "已重新创建目录: " << cachePath << std::endl;

            // 在目录下创建 100 个子目录
            for (int i = 0; i <= 100; ++i) {
                std::string subdir_name = cachePath + "/" + std::to_string(i);
                std::filesystem::create_directory(subdir_name);
            }
            std::cout << "已创建子目录: 0 ~ 100 " << std::endl;
        } catch (const std::exception &e) {
            std::cerr << "发生错误: " << e.what() << std::endl;
            exit(1);
        }

        try {
            cuckoo::brpc_io::RemoteIOServer &server = cuckoo::brpc_io::RemoteIOServer::GetInstance();
            std::string clusterView = config->GetArray(CuckooPropertyKey::CUCKOO_CLUSTER_VIEW);
            std::vector<std::string> views;
            std::stringstream ss(clusterView);
            while (ss.good()) {
                std::string substr;
                getline(ss, substr, ',');
                views.push_back(substr);
            }
            int nodeId = config->GetUint32(CuckooPropertyKey::CUCKOO_NODE_ID);
            server.endPoint = views[nodeId];
            std::cout << "brpc endpoint = " << server.endPoint << std::endl;
            std::thread brpcServerThread(&cuckoo::brpc_io::RemoteIOServer::Run, &server);
            {
                std::unique_lock<std::mutex> lk(server.mutexStart);
                server.cvStart.wait(lk, [&server]() { return server.isStarted; });
            }
            brpcServerThread.detach();
            server.SetReadyFlag();
        } catch (const std::exception &e) {
            std::cerr << "发生错误: " << e.what() << std::endl;
            exit(1);
        }

        ret = CuckooStore::GetInstance()->GetInitStatus();
        if (ret != SUCCESS) {
            println(std::cerr, "init failed, check the log.");
            exit(1);
        }
    }

    static void TearDownTestSuite()
    {
        auto cachePath = config->GetString(CuckooPropertyKey::CUCKOO_CACHE_ROOT);
        if (std::filesystem::exists(cachePath)) {
            std::filesystem::remove_all(cachePath);
            std::cout << "已删除目录及其内容: " << cachePath << std::endl;
        }
        if (writeBuf) {
            free(writeBuf);
            writeBuf = nullptr;
        }
        if (readBuf) {
            free(readBuf);
            readBuf = nullptr;
        }
        if (readBuf2) {
            free(readBuf2);
            readBuf2 = nullptr;
        }
        cuckoo::brpc_io::RemoteIOServer &server = cuckoo::brpc_io::RemoteIOServer::GetInstance();
        server.Stop();
    }

    void SetUp() override {}
    void TearDown() override {}
    void ResetBuf(bool isLarge)
    {
        if (writeBuf) {
            free(writeBuf);
            writeBuf = nullptr;
        }
        if (readBuf) {
            free(readBuf);
            readBuf = nullptr;
        }
        if (readBuf2) {
            free(readBuf2);
            readBuf2 = nullptr;
        }
        if (isLarge) {
            size = config->GetUint32(CuckooPropertyKey::CUCKOO_BIG_FILE_READ_SIZE);
            readSize = config->GetUint32(CuckooPropertyKey::CUCKOO_BLOCK_SIZE);
        } else {
            size = CUCKOO_STORE_STREAM_MAX_SIZE * 2; // make sure write through
            readSize = size / 2;
        }
        // do not consider malloc failure
        writeBuf = (char *)malloc(size);
        memset(writeBuf, 0, size);
        strcpy(writeBuf, "abc\0");
        readBuf = (char *)malloc(readSize);
        memset(readBuf, 0, readSize);
        readBuf2 = (char *)malloc(readSize);
        memset(readBuf2, 0, readSize);
    }

    void NewOpenInstance(size_t inodeId, int nodeId, const std::string &path, int oflags)
    {
        openInstance = std::make_shared<OpenInstance>();
        openInstance->inodeId = inodeId;
        openInstance->nodeId = nodeId;
        openInstance->path = path;
        openInstance->oflags = oflags;
    }

    static std::shared_ptr<CuckooConfig> config;
    static std::shared_ptr<OpenInstance> openInstance;
    static char *writeBuf;
    static size_t size;
    static char *readBuf;
    static size_t readSize;
    static char *readBuf2;
};
