/* Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * SPDX-License-Identifier: MulanPSL-2.0
 */

#include "init/cuckoo_init.h"

#include "conf/cuckoo_property_key.h"
#include "cuckoo_code.h"
#include "log/logging.h"

int32_t CuckooModuleInit::Init()
{
    if (inited) {
        return CUCKOO_SUCCESS;
    }
    std::function<int32_t()> cuckooInitStepOps[] = {[&] { return InnerInit(); },
                                                    [&] { return InitConf(); },
                                                    [&] { return InitLog(); }};

    for (auto &initStep : cuckooInitStepOps) {
        int32_t ret = initStep();
        if (ret != OK) {
            return CUCKOO_ERR_INNER_FAILED;
        }
    }
    inited = true;
    CUCKOO_LOG(LOG_INFO) << "Init CUCKOO client successfully";
    return CUCKOO_SUCCESS;
}

int32_t CuckooModuleInit::InnerInit()
{
    if (cuckooConfig == nullptr) {
        cuckooConfig = std::make_unique<CuckooConfig>();
    }

    return OK;
}

int32_t CuckooModuleInit::InitConf()
{
    if (configDir.empty()) {
        return CUCKOO_IEC_INIT_CONF_FAILED;
    }

    return cuckooConfig->InitConf(configDir);
}

int32_t CuckooModuleInit::InitLog()
{
    auto logMaxSize = cuckooConfig->GetUint32(CuckooPropertyKey::CUCKOO_LOG_MAX_SIZE_MB);
    auto logDir = cuckooConfig->GetString(CuckooPropertyKey::CUCKOO_LOG_DIR);
    if (logDir.empty()) {
        logDir = CUCKOO_DEFAULT_LOG_DIR;
    }

    auto logLevelString = cuckooConfig->GetString(CuckooPropertyKey::CUCKOO_LOG_LEVEL);
    std::unordered_map<std::string, CuckooLogLevel> logLevelMap = {{"TRACE", LOG_TRACE},
                                                                   {"DEBUG", LOG_DEBUG},
                                                                   {"INFO", LOG_INFO},
                                                                   {"WARNING", LOG_WARNING},
                                                                   {"ERROR", LOG_ERROR},
                                                                   {"FATAL", LOG_FATAL}};
    auto logLevel = (logLevelMap.count(logLevelString) ? logLevelMap[logLevelString] : LOG_INFO);

    uint reserved_num = cuckooConfig->GetUint32(CuckooPropertyKey::CUCKOO_LOG_RESERVED_NUM);

    uint reserved_time = cuckooConfig->GetUint32(CuckooPropertyKey::CUCKOO_LOG_RESERVED_TIME);

    int32_t ret =
        CuckooLog::GetInstance()->InitLog(logLevel, GLOGGER, logDir, "cuckoo", logMaxSize, reserved_num, reserved_time);
    if (ret != OK) {
        CUCKOO_LOG(LOG_ERROR) << "Cuckoo init failed caused by init log failed, error code: " << ret;
        return ret;
    }

    CUCKOO_LOG(LOG_INFO) << "Init Cuckoo Log successfully";
    return OK;
}

std::shared_ptr<CuckooConfig> &CuckooModuleInit::GetCuckooConfig() { return cuckooConfig; }

CuckooModuleInit &GetInit()
{
    char *CONFIG_FILE = std::getenv("CONFIG_FILE");
    if (!CONFIG_FILE) {
        CUCKOO_LOG(LOG_ERROR) << "CONFIG_FILE not set";
    }
    std::string configFile = CONFIG_FILE ? CONFIG_FILE : "";
    static CuckooModuleInit instance(configFile);
    return instance;
}
