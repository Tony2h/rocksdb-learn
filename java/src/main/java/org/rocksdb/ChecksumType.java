// Copyright (c) 2011-present, Facebook, Inc.  All rights reserved.
//  This source code is licensed under both the GPLv2 (found in the
//  COPYING file in the root directory) and Apache 2.0 License
//  (found in the LICENSE.Apache file in the root directory).

package org.rocksdb;

/**
 * Checksum types used in conjunction with BlockBasedTable.
 */
public enum ChecksumType {
  /**
   * Not implemented yet.
   */
  kNoChecksum((byte) 0),
  /**
   * CRC32 Checksum
   */
  kCRC32c((byte) 1),
  /**
   * XX Hash
   */
  kxxHash((byte) 2),
  /**
   * XX Hash 64
   */
  kxxHash64((byte) 3),

  kXXH3((byte) 4);

  /**
   * Returns the byte value of the enumerations value
   *
   * @return byte representation
   */
  public byte getValue() {
    return value_;
  }

  ChecksumType(final byte value) {
    value_ = value;
  }

  private final byte value_;
}
