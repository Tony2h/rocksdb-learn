// Copyright (c) 2011-present, Facebook, Inc.  All rights reserved.
//  This source code is licensed under both the GPLv2 (found in the
//  COPYING file in the root directory) and Apache 2.0 License
//  (found in the LICENSE.Apache file in the root directory).
//
// This file implements the "bridge" between Java and C++ for
// ROCKSDB_NAMESPACE::CompressionOptions.

#include <jni.h>

#include "include/org_rocksdb_CompressionOptions.h"
#include "rocksdb/advanced_options.h"
#include "rocksjni/cplusplus_to_java_convert.h"

/*
 * Class:     org_rocksdb_CompressionOptions
 * Method:    newCompressionOptions
 * Signature: ()J
 */
jlong Java_org_rocksdb_CompressionOptions_newCompressionOptions(JNIEnv*,
                                                                jclass) {
  const auto* opt = new ROCKSDB_NAMESPACE::CompressionOptions();
  return GET_CPLUSPLUS_POINTER(opt);
}

/*
 * Class:     org_rocksdb_CompressionOptions
 * Method:    setWindowBits
 * Signature: (JI)V
 */
void Java_org_rocksdb_CompressionOptions_setWindowBits(JNIEnv*, jclass,
                                                       jlong jhandle,
                                                       jint jwindow_bits) {
  auto* opt = reinterpret_cast<ROCKSDB_NAMESPACE::CompressionOptions*>(jhandle);
  opt->window_bits = static_cast<int>(jwindow_bits);
}

/*
 * Class:     org_rocksdb_CompressionOptions
 * Method:    windowBits
 * Signature: (J)I
 */
jint Java_org_rocksdb_CompressionOptions_windowBits(JNIEnv*, jclass,
                                                    jlong jhandle) {
  auto* opt = reinterpret_cast<ROCKSDB_NAMESPACE::CompressionOptions*>(jhandle);
  return static_cast<jint>(opt->window_bits);
}

/*
 * Class:     org_rocksdb_CompressionOptions
 * Method:    setLevel
 * Signature: (JI)V
 */
void Java_org_rocksdb_CompressionOptions_setLevel(JNIEnv*, jclass,
                                                  jlong jhandle, jint jlevel) {
  auto* opt = reinterpret_cast<ROCKSDB_NAMESPACE::CompressionOptions*>(jhandle);
  opt->level = static_cast<int>(jlevel);
}

/*
 * Class:     org_rocksdb_CompressionOptions
 * Method:    level
 * Signature: (J)I
 */
jint Java_org_rocksdb_CompressionOptions_level(JNIEnv*, jclass, jlong jhandle) {
  auto* opt = reinterpret_cast<ROCKSDB_NAMESPACE::CompressionOptions*>(jhandle);
  return static_cast<jint>(opt->level);
}

/*
 * Class:     org_rocksdb_CompressionOptions
 * Method:    setStrategy
 * Signature: (JI)V
 */
void Java_org_rocksdb_CompressionOptions_setStrategy(JNIEnv*, jclass,
                                                     jlong jhandle,
                                                     jint jstrategy) {
  auto* opt = reinterpret_cast<ROCKSDB_NAMESPACE::CompressionOptions*>(jhandle);
  opt->strategy = static_cast<int>(jstrategy);
}

/*
 * Class:     org_rocksdb_CompressionOptions
 * Method:    strategy
 * Signature: (J)I
 */
jint Java_org_rocksdb_CompressionOptions_strategy(JNIEnv*, jclass,
                                                  jlong jhandle) {
  auto* opt = reinterpret_cast<ROCKSDB_NAMESPACE::CompressionOptions*>(jhandle);
  return static_cast<jint>(opt->strategy);
}

/*
 * Class:     org_rocksdb_CompressionOptions
 * Method:    setMaxDictBytes
 * Signature: (JI)V
 */
void Java_org_rocksdb_CompressionOptions_setMaxDictBytes(JNIEnv*, jclass,
                                                         jlong jhandle,
                                                         jint jmax_dict_bytes) {
  auto* opt = reinterpret_cast<ROCKSDB_NAMESPACE::CompressionOptions*>(jhandle);
  opt->max_dict_bytes = static_cast<uint32_t>(jmax_dict_bytes);
}

/*
 * Class:     org_rocksdb_CompressionOptions
 * Method:    maxDictBytes
 * Signature: (J)I
 */
jint Java_org_rocksdb_CompressionOptions_maxDictBytes(JNIEnv*, jclass,
                                                      jlong jhandle) {
  auto* opt = reinterpret_cast<ROCKSDB_NAMESPACE::CompressionOptions*>(jhandle);
  return static_cast<jint>(opt->max_dict_bytes);
}

/*
 * Class:     org_rocksdb_CompressionOptions
 * Method:    setZstdMaxTrainBytes
 * Signature: (JI)V
 */
void Java_org_rocksdb_CompressionOptions_setZstdMaxTrainBytes(
    JNIEnv*, jclass, jlong jhandle, jint jzstd_max_train_bytes) {
  auto* opt = reinterpret_cast<ROCKSDB_NAMESPACE::CompressionOptions*>(jhandle);
  opt->zstd_max_train_bytes = static_cast<uint32_t>(jzstd_max_train_bytes);
}

/*
 * Class:     org_rocksdb_CompressionOptions
 * Method:    zstdMaxTrainBytes
 * Signature: (J)I
 */
jint Java_org_rocksdb_CompressionOptions_zstdMaxTrainBytes(JNIEnv*, jclass,
                                                           jlong jhandle) {
  auto* opt = reinterpret_cast<ROCKSDB_NAMESPACE::CompressionOptions*>(jhandle);
  return static_cast<jint>(opt->zstd_max_train_bytes);
}

/*
 * Class:     org_rocksdb_CompressionOptions
 * Method:    setMaxDictBufferBytes
 * Signature: (JJ)V
 */
void Java_org_rocksdb_CompressionOptions_setMaxDictBufferBytes(
    JNIEnv*, jclass, jlong jhandle, jlong jmax_dict_buffer_bytes) {
  auto* opt = reinterpret_cast<ROCKSDB_NAMESPACE::CompressionOptions*>(jhandle);
  opt->max_dict_buffer_bytes = static_cast<uint64_t>(jmax_dict_buffer_bytes);
}

/*
 * Class:     org_rocksdb_CompressionOptions
 * Method:    maxDictBufferBytes
 * Signature: (J)J
 */
jlong Java_org_rocksdb_CompressionOptions_maxDictBufferBytes(JNIEnv*, jclass,
                                                             jlong jhandle) {
  auto* opt = reinterpret_cast<ROCKSDB_NAMESPACE::CompressionOptions*>(jhandle);
  return static_cast<jlong>(opt->max_dict_buffer_bytes);
}

/*
 * Class:     org_rocksdb_CompressionOptions
 * Method:    setZstdMaxTrainBytes
 * Signature: (JZ)V
 */
void Java_org_rocksdb_CompressionOptions_setUseZstdDictTrainer(
    JNIEnv*, jclass, jlong jhandle, jboolean juse_zstd_dict_trainer) {
  auto* opt = reinterpret_cast<ROCKSDB_NAMESPACE::CompressionOptions*>(jhandle);
  opt->use_zstd_dict_trainer = juse_zstd_dict_trainer == JNI_TRUE;
}

/*
 * Class:     org_rocksdb_CompressionOptions
 * Method:    zstdMaxTrainBytes
 * Signature: (J)Z
 */
jboolean Java_org_rocksdb_CompressionOptions_useZstdDictTrainer(JNIEnv*, jclass,
                                                                jlong jhandle) {
  auto* opt = reinterpret_cast<ROCKSDB_NAMESPACE::CompressionOptions*>(jhandle);
  return static_cast<bool>(opt->use_zstd_dict_trainer);
}

/*
 * Class:     org_rocksdb_CompressionOptions
 * Method:    setEnabled
 * Signature: (JZ)V
 */
void Java_org_rocksdb_CompressionOptions_setEnabled(JNIEnv*, jclass,
                                                    jlong jhandle,
                                                    jboolean jenabled) {
  auto* opt = reinterpret_cast<ROCKSDB_NAMESPACE::CompressionOptions*>(jhandle);
  opt->enabled = jenabled == JNI_TRUE;
}

/*
 * Class:     org_rocksdb_CompressionOptions
 * Method:    enabled
 * Signature: (J)Z
 */
jboolean Java_org_rocksdb_CompressionOptions_enabled(JNIEnv*, jclass,
                                                     jlong jhandle) {
  auto* opt = reinterpret_cast<ROCKSDB_NAMESPACE::CompressionOptions*>(jhandle);
  return static_cast<bool>(opt->enabled);
}
/*
 * Class:     org_rocksdb_CompressionOptions
 * Method:    disposeInternal
 * Signature: (J)V
 */
void Java_org_rocksdb_CompressionOptions_disposeInternalJni(JNIEnv*, jclass,
                                                            jlong jhandle) {
  delete reinterpret_cast<ROCKSDB_NAMESPACE::CompressionOptions*>(jhandle);
}
