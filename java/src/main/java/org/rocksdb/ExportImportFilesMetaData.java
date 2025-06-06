//  Copyright (c) Meta Platforms, Inc. and affiliates.
//
//  This source code is licensed under both the GPLv2 (found in the
//  COPYING file in the root directory) and Apache 2.0 License
//  (found in the LICENSE.Apache file in the root directory).

package org.rocksdb;

/**
 * The metadata that describes a column family.
 */
public class ExportImportFilesMetaData extends RocksObject {
  ExportImportFilesMetaData(final long nativeHandle) {
    super(nativeHandle);
  }

  @Override protected native void disposeInternal(final long handle);
}
