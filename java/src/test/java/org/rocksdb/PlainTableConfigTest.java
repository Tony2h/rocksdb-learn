// Copyright (c) 2011-present, Facebook, Inc.  All rights reserved.
//  This source code is licensed under both the GPLv2 (found in the
//  COPYING file in the root directory) and Apache 2.0 License
//  (found in the LICENSE.Apache file in the root directory).

package org.rocksdb;

import org.junit.ClassRule;
import org.junit.Test;

import static org.assertj.core.api.Assertions.assertThat;

public class PlainTableConfigTest {

  @ClassRule
  public static final RocksNativeLibraryResource ROCKS_NATIVE_LIBRARY_RESOURCE =
      new RocksNativeLibraryResource();

  @Test
  public void keySize() {
    final PlainTableConfig plainTableConfig = new PlainTableConfig();
    plainTableConfig.setKeySize(5);
    assertThat(plainTableConfig.keySize()).
        isEqualTo(5);
  }

  @Test
  public void bloomBitsPerKey() {
    final PlainTableConfig plainTableConfig = new PlainTableConfig();
    plainTableConfig.setBloomBitsPerKey(11);
    assertThat(plainTableConfig.bloomBitsPerKey()).
        isEqualTo(11);
  }

  @Test
  public void hashTableRatio() {
    final PlainTableConfig plainTableConfig = new PlainTableConfig();
    plainTableConfig.setHashTableRatio(0.95);
    assertThat(plainTableConfig.hashTableRatio()).
        isEqualTo(0.95);
  }

  @Test
  public void indexSparseness() {
    final PlainTableConfig plainTableConfig = new PlainTableConfig();
    plainTableConfig.setIndexSparseness(18);
    assertThat(plainTableConfig.indexSparseness()).
        isEqualTo(18);
  }

  @Test
  public void hugePageTlbSize() {
    final PlainTableConfig plainTableConfig = new PlainTableConfig();
    plainTableConfig.setHugePageTlbSize(1);
    assertThat(plainTableConfig.hugePageTlbSize()).
        isEqualTo(1);
  }

  @Test
  public void encodingType() {
    final PlainTableConfig plainTableConfig = new PlainTableConfig();
    plainTableConfig.setEncodingType(EncodingType.kPrefix);
    assertThat(plainTableConfig.encodingType()).isEqualTo(
        EncodingType.kPrefix);
  }

  @Test
  public void fullScanMode() {
    final PlainTableConfig plainTableConfig = new PlainTableConfig();
    plainTableConfig.setFullScanMode(true);
    assertThat(plainTableConfig.fullScanMode()).isTrue();  }

  @Test
  public void storeIndexInFile() {
    final PlainTableConfig plainTableConfig = new PlainTableConfig();
    plainTableConfig.setStoreIndexInFile(true);
    assertThat(plainTableConfig.storeIndexInFile()).
        isTrue();
  }

  @Test
  public void plainTableConfig() {
    try(final Options opt = new Options()) {
      final PlainTableConfig plainTableConfig = new PlainTableConfig();
      opt.setTableFormatConfig(plainTableConfig);
      assertThat(opt.tableFactoryName()).isEqualTo("PlainTable");
    }
  }
}
