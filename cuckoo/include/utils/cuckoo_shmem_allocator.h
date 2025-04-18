/* Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * SPDX-License-Identifier: MulanPSL-2.0
 */

#ifndef CUCKOO_SHMEM_ALLOCATOR_H
#define CUCKOO_SHMEM_ALLOCATOR_H

#ifdef __cplusplus
#include <atomic>
using std::atomic_uint_fast64_t;
#else
#include <stdatomic.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define CUCKOO_SHMEM_ALLOCATOR_STATE_BIT_COUNT          64
// 2^6 = 64 -> 7 kind of size
#define CUCKOO_SHMEM_ALLOCATOR_FREE_LIST_COUNT          7

#define CUCKOO_SHMEM_ALLOCATOR_MAX_SUPPORT_ALLOC_SIZE   (1024 * 1024)
#define CUCKOO_SHMEM_ALLOCATOR_MIN_SUPPORT_ALLOC_SIZE   (CUCKOO_SHMEM_ALLOCATOR_MAX_SUPPORT_ALLOC_SIZE / CUCKOO_SHMEM_ALLOCATOR_STATE_BIT_COUNT)

#define CUCKOO_SHMEM_ALLOCATOR_PAGE_SIZE                CUCKOO_SHMEM_ALLOCATOR_MAX_SUPPORT_ALLOC_SIZE
#define CUCKOO_SHMEM_ALLOCATOR_MIN_BLOCK_SIZE           CUCKOO_SHMEM_ALLOCATOR_MIN_SUPPORT_ALLOC_SIZE
#define CUCKOO_SHMEM_ALLOCATOR_PAD_SIZE                 128
typedef union PaddedAtomic64 
{
    atomic_uint_fast64_t data;
    char padding[CUCKOO_SHMEM_ALLOCATOR_PAD_SIZE];
} PaddedAtomic64;

typedef struct CuckooShmemAllocator 
{
    char* shmem;
    uint64_t size;
    uint32_t pageCount;

    //located in shmem
    PaddedAtomic64* signatureCounter;
    PaddedAtomic64* freeListHint;
    PaddedAtomic64* pageCntlArray;
    char* allocatableSpaceBase;
} CuckooShmemAllocator;


#define CUCKOO_SHMEM_ALLOCATOR_GET_POINTER(allocator, shift) ((allocator)->allocatableSpaceBase + (shift))
#define CUCKOO_SHMEM_ALLOCATOR_POINTER_GET_SIZE(pointer) (((MemoryHdr*)((char*)(pointer) - sizeof(MemoryHdr)))->size)
#define CUCKOO_SHMEM_ALLOCATOR_SET_SIGNATURE(pointer, sign) (((MemoryHdr*)((char*)(pointer) - sizeof(MemoryHdr)))->signature = (sign))
#define CUCKOO_SHMEM_ALLOCATOR_GET_SIGNATURE(pointer) (((MemoryHdr*)((char*)(pointer) - sizeof(MemoryHdr)))->signature)

int CuckooShmemAllocatorInit(CuckooShmemAllocator *allocator, char *shmem, uint64_t size);

int64_t CuckooShmemAllocatorGetUniqueSignature(CuckooShmemAllocator *allocator);

typedef struct MemoryHdr 
{
    int64_t signature;
    uint64_t size;
    uint64_t capacity;
} MemoryHdr;
uint64_t CuckooShmemAllocatorMalloc(CuckooShmemAllocator *allocator, uint64_t size);

void CuckooShmemAllocatorFree(CuckooShmemAllocator *allocator, uint64_t shift);

#ifdef __cplusplus
}
#endif

#endif
