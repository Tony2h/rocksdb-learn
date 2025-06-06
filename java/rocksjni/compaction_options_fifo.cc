// Copyright (c) 2011-present, Facebook, Inc.  All rights reserved.
//  This source code is licensed under both the GPLv2 (found in the
//  COPYING file in the root directory) and Apache 2.0 License
//  (found in the LICENSE.Apache file in the root directory).
//
// This file implements the "bridge" between Java and C++ for
// ROCKSDB_NAMESPACE::CompactionOptionsFIFO.

#include <jni.h>

#include "include/org_rocksdb_CompactionOptionsFIFO.h"
#include "rocksdb/advanced_options.h"
#include "rocksjni/cplusplus_to_java_convert.h"

/*
 * Class:     org_rocksdb_CompactionOptionsFIFO
 * Method:    newCompactionOptionsFIFO
 * Signature: ()J
 */
jlong Java_org_rocksdb_CompactionOptionsFIFO_newCompactionOptionsFIFO(JNIEnv*,
                                                                      jclass) {
  const auto* opt = new ROCKSDB_NAMESPACE::CompactionOptionsFIFO();
  return GET_CPLUSPLUS_POINTER(opt);
}

/*
 * Class:     org_rocksdb_CompactionOptionsFIFO
 * Method:    setMaxTableFilesSize
 * Signature: (JJ)V
 */
void Java_org_rocksdb_CompactionOptionsFIFO_setMaxTableFilesSize(
    JNIEnv*, jclass, jlong jhandle, jlong jmax_table_files_size) {
  auto* opt =
      reinterpret_cast<ROCKSDB_NAMESPACE::CompactionOptionsFIFO*>(jhandle);
  opt->max_table_files_size = static_cast<uint64_t>(jmax_table_files_size);
}

/*
 * Class:     org_rocksdb_CompactionOptionsFIFO
 * Method:    maxTableFilesSize
 * Signature: (J)J
 */
jlong Java_org_rocksdb_CompactionOptionsFIFO_maxTableFilesSize(JNIEnv*, jclass,
                                                               jlong jhandle) {
  auto* opt =
      reinterpret_cast<ROCKSDB_NAMESPACE::CompactionOptionsFIFO*>(jhandle);
  return static_cast<jlong>(opt->max_table_files_size);
}

/*
 * Class:     org_rocksdb_CompactionOptionsFIFO
 * Method:    setAllowCompaction
 * Signature: (JZ)V
 */
void Java_org_rocksdb_CompactionOptionsFIFO_setAllowCompaction(
    JNIEnv*, jclass, jlong jhandle, jboolean allow_compaction) {
  auto* opt =
      reinterpret_cast<ROCKSDB_NAMESPACE::CompactionOptionsFIFO*>(jhandle);
  opt->allow_compaction = static_cast<bool>(allow_compaction);
}

/*
 * Class:     org_rocksdb_CompactionOptionsFIFO
 * Method:    allowCompaction
 * Signature: (J)Z
 */
jboolean Java_org_rocksdb_CompactionOptionsFIFO_allowCompaction(JNIEnv*, jclass,
                                                                jlong jhandle) {
  auto* opt =
      reinterpret_cast<ROCKSDB_NAMESPACE::CompactionOptionsFIFO*>(jhandle);
  return static_cast<jboolean>(opt->allow_compaction);
}

/*
 * Class:     org_rocksdb_CompactionOptionsFIFO
 * Method:    disposeInternal
 * Signature: (J)V
 */
void Java_org_rocksdb_CompactionOptionsFIFO_disposeInternalJni(JNIEnv*, jclass,
                                                               jlong jhandle) {
  delete reinterpret_cast<ROCKSDB_NAMESPACE::CompactionOptionsFIFO*>(jhandle);
}
