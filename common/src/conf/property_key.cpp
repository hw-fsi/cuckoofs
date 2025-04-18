/* Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * SPDX-License-Identifier: MulanPSL-2.0
 */

#include "conf/property_key.h"

#include "log/logging.h"

std::shared_ptr<PropertyKey> PropertyKey::Builder::build()
{
    std::shared_ptr<PropertyKey> key;
    try {
        key = std::make_shared<PropertyKey>(category, name, scope, dataType);
    } catch (const std::bad_alloc &e) {
        CUCKOO_LOG(LOG_ERROR) << "Alloc memory for conf property key failed, error msg: " << e.what() << " size is: . "
                              << sizeof(PropertyKey);
        key = nullptr;
    }
    PropertyKey::keyMap.insert(std::pair(name, key));
    return key;
}

DataType PropertyKey::GetDataType() const { return dataType_; }

std::string PropertyKey::GetName() const { return name_; }

const std::string &PropertyKey::GetCategory() const { return category_; }

Scope PropertyKey::GetScope() const { return scope_; }

bool PropertyKey::GetIsDynamic() const { return category_ == "runtime"; }

const std::function<void(std::any)> &PropertyKey::GetUpdater() const { return updater_; }

void PropertyKey::SetUpdater(const std::function<void(std::any)> &updater) { updater_ = updater; }
