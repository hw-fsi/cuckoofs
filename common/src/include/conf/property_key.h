/* Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * SPDX-License-Identifier: MulanPSL-2.0
 */

#pragma once

#include <any>
#include <functional>
#include <map>
#include <memory>
#include <string>

enum DataType { CUCKOO_STRING = 1, CUCKOO_UINT, CUCKOO_BOOL, CUCKOO_ARRAY, CUCKOO_UINT64, CUCKOO_DOUBLE };

enum Scope { CUCKOO = 1 };

class PropertyKey {
  public:
    PropertyKey(const std::string &category, const std::string &name, Scope scope, DataType dataType)
        : category_(category),
          name_(name),
          scope_(scope),
          dataType_(dataType)
    {
    }

    DataType GetDataType() const;

    std::string GetName() const;

    const std::string &GetCategory() const;

    Scope GetScope() const;

    bool GetIsDynamic() const;

    const std::function<void(std::any)> &GetUpdater() const;

    void SetUpdater(const std::function<void(std::any)> &updater);

    inline static std::map<std::string, std::shared_ptr<PropertyKey>> keyMap{};

  protected:
    class Builder {
      public:
        Builder(const std::string &category, const std::string &name, Scope scope, DataType dataType)
            : category(category),
              name(name),
              scope(scope),
              dataType(dataType)
        {
        }

        std::shared_ptr<PropertyKey> build();

      private:
        std::string category;
        std::string name;
        Scope scope;
        DataType dataType;
    };

  private:
    std::string category_;
    std::string name_;
    Scope scope_;
    DataType dataType_;
    std::function<void(std::any)> updater_{nullptr};
};
