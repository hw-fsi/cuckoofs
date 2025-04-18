/* Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * SPDX-License-Identifier: MulanPSL-2.0
 */

#include "conf/cuckoo_config.h"

#include <securec.h>
#include <fstream>

#include "cuckoo_code.h"
#include "log/logging.h"

int32_t CuckooConfig::InitConf(const std::string &file)
{
    int32_t ret = LoadConfig(file);
    if (ret != OK) {
        return ret;
    }

    CUCKOO_LOG(LOG_INFO) << "Init CUCKOO configuration successfully";
    return OK;
}

int32_t CuckooConfig::LoadConfig(const std::string &file)
{
    int32_t ret = ReadJsonFile(file);
    if (ret != OK) {
        return ret;
    }

    auto properties = ParseJsonConfig(Scope::CUCKOO);
    if (!properties.has_value()) {
        CUCKOO_LOG(LOG_ERROR) << "Init Cuckoo config failed, caused by parse config failed.";
        return CUCKOO_IEC_INIT_CONF_FAILED;
    }

    propertiesMap.insert(properties->begin(), properties->end());

    return OK;
}

int32_t CuckooConfig::ReadJsonFile(const std::string &file)
{
    if (file.empty()) {
        CUCKOO_LOG(LOG_ERROR) << "Init Cuckoo config failed, file is empty.";
        return CUCKOO_IEC_INIT_CONF_FAILED;
    }

    char realFilePath[PATH_MAX];
    errno_t err = memset_s(realFilePath, sizeof(realFilePath), 0, sizeof(realFilePath));
    if (err != 0) {
        CUCKOO_LOG(LOG_ERROR) << "Secure func failed: " << err;
        return CUCKOO_IEC_INIT_CONF_FAILED;
    }

    auto formatRet = realpath(file.c_str(), realFilePath);
    if (formatRet == nullptr) {
        CUCKOO_LOG(LOG_ERROR) << "Normalize file failed with error: " << strerror(errno);
        CUCKOO_LOG(LOG_ERROR) << "config file path: " << file;
        return CUCKOO_IEC_INIT_CONF_FAILED;
    }

    std::ifstream conf(realFilePath);
    if (!conf.is_open()) {
        CUCKOO_LOG(LOG_ERROR) << "Init CUCKOO config failed, open file: " << file.c_str() << " failed.";
        return CUCKOO_IEC_INIT_CONF_FAILED;
    }

    Json::CharReaderBuilder builder;
    builder["collectComments"] = false;
    JSONCPP_STRING errs;

    bool parseRet = Json::parseFromStream(builder, conf, &root, &errs);
    if (!parseRet || !errs.empty()) {
        CUCKOO_LOG(LOG_ERROR) << "Init CUCKOO config failed, parse config from steam failed, file is " << file.c_str();
        return CUCKOO_IEC_INIT_CONF_FAILED;
    }

    return OK;
}

std::any CuckooConfig::LookUpProperty(const std::shared_ptr<PropertyKey> &key)
{
    std::shared_lock<std::shared_mutex> readLock(mutex_);
    auto iter = propertiesMap.find(key);
    if (iter == propertiesMap.end()) {
        CUCKOO_LOG(LOG_ERROR) << "Property " << key->GetName() << " not exist.";
        return std::any();
    }
    return iter->second;
}

std::string CuckooConfig::GetArray(const std::shared_ptr<PropertyKey> &key)
{
    auto value = LookUpProperty(key);
    if (!value.has_value() || value.type() != typeid(std::string)) {
        CUCKOO_LOG(LOG_ERROR) << "Get array property failed, " << key->GetName() << " if of type "
                              << key->GetDataType();
        return "";
    }
    return std::any_cast<std::string>(value);
}

uint32_t CuckooConfig::GetUint32(const std::shared_ptr<PropertyKey> &key)
{
    auto value = LookUpProperty(key);
    if (!value.has_value() || value.type() != typeid(uint32_t)) {
        CUCKOO_LOG(LOG_ERROR) << "Get uint32 property failed, " << key->GetName() << " if of type "
                              << key->GetDataType();
        return 0U;
    }
    return std::any_cast<uint32_t>(value);
}

uint64_t CuckooConfig::GetUint64(const std::shared_ptr<PropertyKey> &key)
{
    auto value = LookUpProperty(key);
    if (!value.has_value() || value.type() != typeid(uint64_t)) {
        CUCKOO_LOG(LOG_ERROR) << "Get uint64 property failed, " << key->GetName() << " if of type "
                              << key->GetDataType();
        return 0UL;
    }
    return std::any_cast<uint64_t>(value);
}

std::string CuckooConfig::GetString(const std::shared_ptr<PropertyKey> &key)
{
    auto value = LookUpProperty(key);
    if (!value.has_value() || value.type() != typeid(std::string)) {
        CUCKOO_LOG(LOG_ERROR) << "Get string property failed, " << key->GetName() << " if of type "
                              << key->GetDataType();
        return "";
    }
    return std::any_cast<std::string>(value);
}

double CuckooConfig::GetDouble(const std::shared_ptr<PropertyKey> &key)
{
    auto value = LookUpProperty(key);
    if (!value.has_value() || value.type() != typeid(double)) {
        CUCKOO_LOG(LOG_ERROR) << "Get double property failed, " << key->GetName() << " if of type "
                              << key->GetDataType();
        return 0.0;
    }
    return std::any_cast<double>(value);
}

bool CuckooConfig::GetBool(const std::shared_ptr<PropertyKey> &key)
{
    auto value = LookUpProperty(key);
    if (!value.has_value() || value.type() != typeid(bool)) {
        CUCKOO_LOG(LOG_ERROR) << "Get bool property failed, " << key->GetName() << " if of type " << key->GetDataType();
        return false;
    }
    return std::any_cast<bool>(value);
}

void ParseJsonTree(const Json::Value &base, std::map<std::string, Json::Value> &jsonKvMap)
{
    auto members = base.getMemberNames();
    for (const auto &key : members) {
        const Json::Value &value = base[key];
        jsonKvMap[key] = value;
    }
}

std::optional<CuckooConfig::PropertyMapType> CuckooConfig::ParseJsonConfig(Scope scope)
{
    Json::Value mainBaseValue = root["main"];
    Json::Value runtimeBaseValue = root["runtime"];
    std::map<std::string, Json::Value> jsonKvMap;
    ParseJsonTree(mainBaseValue, jsonKvMap);
    ParseJsonTree(runtimeBaseValue, jsonKvMap);

    std::map<const std::shared_ptr<PropertyKey>, std::any> properties;

    for (const auto &kv : PropertyKey::keyMap) {
        std::string keyString = kv.first;
        auto propertyKey = kv.second;
        if (propertyKey == nullptr) {
            CUCKOO_LOG(LOG_ERROR) << "Get property key failed.";
            return std::nullopt;
        }
        if (propertyKey->GetScope() != scope) {
            continue;
        }
        auto jsonMapItr = jsonKvMap.find(keyString);
        if (jsonMapItr == jsonKvMap.end()) {
            CUCKOO_LOG(LOG_ERROR) << "Parse json config failed, property " << keyString.c_str() << " not exist.";
            return std::nullopt;
        }

        std::any value = FormatUtil::JsonToAny(jsonMapItr->second, propertyKey->GetDataType());
        if (!value.has_value()) {
            CUCKOO_LOG(LOG_ERROR) << "Parse json config failed, property " << keyString.c_str() << " should be of type "
                                  << propertyKey->GetDataType();
            return std::nullopt;
        }
        properties.insert(std::pair<std::shared_ptr<PropertyKey>, std::any>(propertyKey, value));
    }
    return properties;
}

std::any FormatUtil::ParseJsonArray(const Json::Value &value)
{
    std::string tmp;
    if (!value.empty()) {
        for (const auto &element : value) {
            tmp.append(element.asString() + ',');
        }
        tmp.pop_back();
    }
    return std::any(tmp);
}

std::any FormatUtil::JsonToAny(const Json::Value &value, DataType dataType)
{
    auto search = JsonToAnyConverter.find(dataType);
    return (search != JsonToAnyConverter.end()) ? JsonToAnyConverter[dataType](value) : std::any();
}

std::any FormatUtil::StringToAny(const std::string &value, DataType dataType)
{
    auto search = StringToAnyConverter.find(dataType);
    return (search != StringToAnyConverter.end()) ? StringToAnyConverter[dataType](value) : std::any();
}

std::string FormatUtil::AnyToString(const std::any &value, DataType dataType)
{
    auto search = AnyToStringConverter.find(dataType);
    return (search != AnyToStringConverter.end()) ? AnyToStringConverter[dataType](value) : std::string();
}

std::map<DataType, FormatUtil::JsonToAnyFunc> FormatUtil::JsonToAnyConverter = {
    {CUCKOO_STRING,
     [](const Json::Value &value) { return value.isString() ? std::any(value.asString()) : std::any(); }},
    {CUCKOO_UINT, [](const Json::Value &value) { return value.isUInt() ? std::any(value.asUInt()) : std::any(); }},
    {CUCKOO_BOOL, [](const Json::Value &value) { return value.isBool() ? std::any(value.asBool()) : std::any(); }},
    {CUCKOO_ARRAY, [](const Json::Value &value) { return value.isArray() ? ParseJsonArray(value) : std::any(); }},
    {CUCKOO_UINT64,
     [](const Json::Value &value) { return value.isUInt64() ? std::any(value.asUInt64()) : std::any(); }},
    {CUCKOO_DOUBLE,
     [](const Json::Value &value) { return value.isDouble() ? std::any(value.asDouble()) : std::any(); }}};

std::map<DataType, FormatUtil::StringToAnyFunc> FormatUtil::StringToAnyConverter = {
    {CUCKOO_STRING, [](const std::string &value) { return std::any(value); }},
    {CUCKOO_UINT, [](const std::string &value) { return std::any(std::stoul(value)); }},
    {CUCKOO_BOOL, [](const std::string &value) { return std::any(value != "0"); }},
    {CUCKOO_ARRAY, [](const std::string &value) { return std::any(value); }},
    {CUCKOO_UINT64, [](const std::string &value) { return std::any(std::stoull(value)); }},
    {CUCKOO_DOUBLE, [](const std::string &value) { return std::any(std::stod(value)); }}};

std::map<DataType, FormatUtil::AnyToStringFunc> FormatUtil::AnyToStringConverter = {
    {CUCKOO_STRING, [](const std::any &value) { return std::any_cast<std::string>(value); }},
    {CUCKOO_UINT, [](const std::any &value) { return std::to_string(std::any_cast<uint32_t>(value)); }},
    {CUCKOO_BOOL, [](const std::any &value) { return std::to_string(std::any_cast<bool>(value)); }},
    {CUCKOO_ARRAY, [](const std::any &value) { return std::any_cast<std::string>(value); }},
    {CUCKOO_UINT64, [](const std::any &value) { return std::to_string(std::any_cast<uint64_t>(value)); }},
    {CUCKOO_DOUBLE, [](const std::any &value) { return std::to_string(std::any_cast<double>(value)); }}};
