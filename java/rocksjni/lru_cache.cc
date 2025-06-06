// Copyright (c) 2011-present, Facebook, Inc.  All rights reserved.
//  This source code is licensed under both the GPLv2 (found in the
//  COPYING file in the root directory) and Apache 2.0 License
//  (found in the LICENSE.Apache file in the root directory).
//
// This file implements the "bridge" between Java and C++ for
// ROCKSDB_NAMESPACE::LRUCache.

#include "cache/lru_cache.h"

#include <jni.h>

#include "include/org_rocksdb_LRUCache.h"
#include "rocksjni/cplusplus_to_java_convert.h"

/*
 * Class:     org_rocksdb_LRUCache
 * Method:    newLRUCache
 * Signature: (JIZD)J
 */
jlong Java_org_rocksdb_LRUCache_newLRUCache(JNIEnv* /*env*/, jclass /*jcls*/,
                                            jlong jcapacity,
                                            jint jnum_shard_bits,
                                            jboolean jstrict_capacity_limit,
                                            jdouble jhigh_pri_pool_ratio,
                                            jdouble jlow_pri_pool_ratio) {
  auto* sptr_lru_cache = new std::shared_ptr<ROCKSDB_NAMESPACE::Cache>(
      ROCKSDB_NAMESPACE::NewLRUCache(
          static_cast<size_t>(jcapacity), static_cast<int>(jnum_shard_bits),
          static_cast<bool>(jstrict_capacity_limit),
          static_cast<double>(jhigh_pri_pool_ratio),
          nullptr /* memory_allocator */, rocksdb::kDefaultToAdaptiveMutex,
          rocksdb::kDefaultCacheMetadataChargePolicy,
          static_cast<double>(jlow_pri_pool_ratio)));
  return GET_CPLUSPLUS_POINTER(sptr_lru_cache);
}

/*
 * Class:     org_rocksdb_LRUCache
 * Method:    disposeInternal
 * Signature: (J)V
 */
void Java_org_rocksdb_LRUCache_disposeInternalJni(JNIEnv* /*env*/,
                                                  jclass /*jcls*/,
                                                  jlong jhandle) {
  auto* sptr_lru_cache =
      reinterpret_cast<std::shared_ptr<ROCKSDB_NAMESPACE::Cache>*>(jhandle);
  delete sptr_lru_cache;  // delete std::shared_ptr
}
