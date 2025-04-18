/* Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * SPDX-License-Identifier: MulanPSL-2.0
 */

#include "buffer/open_instance.h"

void OpenInstance::LockOpenInstance() { fileMutex.lock(); }
void OpenInstance::UnlockOpenInstance() { fileMutex.unlock(); }
