/* Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * SPDX-License-Identifier: MulanPSL-2.0
 */

#ifndef CUCKOO_METADB_META_SERIALIZE_INTERFACE_HELPER_H
#define CUCKOO_METADB_META_SERIALIZE_INTERFACE_HELPER_H

#include <stdint.h>

#include "metadb/meta_handle.h"

#ifdef __cplusplus
extern "C" {
#endif

CuckooSupportMetaService MetaServiceTypeDecode(int32_t type);
int32_t MetaServiceTypeEncode(CuckooSupportMetaService metaService);

bool SerializedDataMetaParamDecode(CuckooSupportMetaService metaService, int count, 
                                SerializedData *param, MetaProcessInfoData *infoArray);

bool SerializedDataMetaParamEncodeWithPerProcessFlatBufferBuilder(
    CuckooSupportMetaService metaService, MetaProcessInfo *infoArray, int32_t* index,
    int count, SerializedData *param);

bool SerializedDataMetaResponseDecode(CuckooSupportMetaService metaService, int count, 
                                SerializedData *response, MetaProcessInfoData *infoArray);

bool SerializedDataMetaResponseEncodeWithPerProcessFlatBufferBuilder(
    CuckooSupportMetaService metaService, int count, MetaProcessInfoData *infoArray,
    SerializedData *response);

#ifdef __cplusplus
}
#endif

#endif