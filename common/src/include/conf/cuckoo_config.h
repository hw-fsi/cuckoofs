/* Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * SPDX-License-Identifier: MulanPSL-2.0
 */

#pragma once

#include <limits.h>
#include <any>
#include <cstring>
#include <map>
#include <optional>
#include <shared_mutex>

#include <json/json.h>

#include "property_key.h"

class CuckooConfig {
    using PropertyMapType = std::map<const std::shared_ptr<PropertyKey>, std::any>;

  public:
    CuckooConfig() = default;

    ~CuckooConfig() = default;

    CuckooConfig(const CuckooConfig &) = delete;

    CuckooConfig &operator=(const CuckooConfig &) = delete;

    int32_t InitConf(const std::string &file);

    uint32_t GetUint32(const std::shared_ptr<PropertyKey> &key);

    uint64_t GetUint64(const std::shared_ptr<PropertyKey> &key);

    std::string GetString(const std::shared_ptr<PropertyKey> &key);

    double GetDouble(const std::shared_ptr<PropertyKey> &key);

    bool GetBool(const std::shared_ptr<PropertyKey> &key);

    std::string GetArray(const std::shared_ptr<PropertyKey> &key);

  protected:
    Json::Value root;

    std::map<const std::shared_ptr<PropertyKey>, std::any> propertiesMap;

  private:
    int32_t LoadConfig(const std::string &file);

    int32_t ReadJsonFile(const std::string &file);

    std::optional<PropertyMapType> ParseJsonConfig(Scope scope);

    std::any LookUpProperty(const std::shared_ptr<PropertyKey> &key);

    mutable std::shared_mutex mutex_;
};

class FormatUtil {
    using JsonToAnyFunc = std::function<std::any(const Json::Value &value)>;
    using StringToAnyFunc = std::function<std::any(const std::string &value)>;
    using AnyToStringFunc = std::function<std::string(const std::any &value)>;

  public:
    static std::any JsonToAny(const Json::Value &value, DataType dataType);

    static std::any StringToAny(const std::string &value, DataType dataType);

    static std::string AnyToString(const std::any &value, DataType dataType);

  private:
    static std::any ParseJsonArray(const Json::Value &value);

    static std::map<DataType, JsonToAnyFunc> JsonToAnyConverter;

    static std::map<DataType, StringToAnyFunc> StringToAnyConverter;

    static std::map<DataType, AnyToStringFunc> AnyToStringConverter;
};
