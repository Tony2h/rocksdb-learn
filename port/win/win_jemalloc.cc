//  Copyright (c) 2011-present, Facebook, Inc.  All rights reserved.
//  This source code is licensed under both the GPLv2 (found in the
//  COPYING file in the root directory) and Apache 2.0 License
//  (found in the LICENSE.Apache file in the root directory).
//
// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#if defined(OS_WIN)

#ifndef ROCKSDB_JEMALLOC
#error This file can only be part of jemalloc aware build
#endif

#include <stdexcept>

#include "jemalloc/jemalloc.h"
#include "port/win/port_win.h"

#if defined(ZSTD) && defined(ZSTD_STATIC_LINKING_ONLY)
#include <zstd.h>
#if (ZSTD_VERSION_NUMBER >= 500)
namespace ROCKSDB_NAMESPACE {
namespace port {
void* JemallocAllocateForZSTD(void* /* opaque */, size_t size) {
  return je_malloc(size);
}
void JemallocDeallocateForZSTD(void* /* opaque */, void* address) {
  je_free(address);
}
ZSTD_customMem GetJeZstdAllocationOverrides() {
  return {JemallocAllocateForZSTD, JemallocDeallocateForZSTD, nullptr};
}
}  // namespace port
}  // namespace ROCKSDB_NAMESPACE
#endif  // (ZSTD_VERSION_NUMBER >= 500)
#endif  // defined(ZSTD) defined(ZSTD_STATIC_LINKING_ONLY)

// Global operators to be replaced by a linker when this file is
// a part of the build

namespace ROCKSDB_NAMESPACE {
namespace port {
void* jemalloc_aligned_alloc(size_t size, size_t alignment) noexcept {
  return je_aligned_alloc(alignment, size);
}
void jemalloc_aligned_free(void* p) noexcept { je_free(p); }
}  // namespace port
}  // namespace ROCKSDB_NAMESPACE

void* operator new(size_t size) {
  void* p = je_malloc(size);
  if (!p) {
    throw std::bad_alloc();
  }
  return p;
}

void* operator new[](size_t size) {
  void* p = je_malloc(size);
  if (!p) {
    throw std::bad_alloc();
  }
  return p;
}

void operator delete(void* p) {
  if (p) {
    je_free(p);
  }
}

void operator delete[](void* p) {
  if (p) {
    je_free(p);
  }
}

#endif
