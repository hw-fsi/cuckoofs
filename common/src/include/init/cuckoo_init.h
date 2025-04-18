/* Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * SPDX-License-Identifier: MulanPSL-2.0
 */

#pragma once

#include "conf/cuckoo_config.h"

class CuckooModuleInit {
  public:
    explicit CuckooModuleInit()
        : cuckooConfig(nullptr)
    {
    }
    explicit CuckooModuleInit(const std::string &conf)
        : cuckooConfig(nullptr),
          configDir(conf)
    {
    }

    ~CuckooModuleInit() = default;

    int32_t Init();
    int32_t InnerInit();
    int32_t InitConf();
    int32_t InitLog();
    std::shared_ptr<CuckooConfig> &GetCuckooConfig();

  protected:
    std::shared_ptr<CuckooConfig> cuckooConfig;
    std::string configDir;
    bool inited = false;
};

CuckooModuleInit &GetInit();
