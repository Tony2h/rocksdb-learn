// Copyright (c) 2011-present, Facebook, Inc.  All rights reserved.
//  This source code is licensed under both the GPLv2 (found in the
//  COPYING file in the root directory) and Apache 2.0 License
//  (found in the LICENSE.Apache file in the root directory).

package org.rocksdb;

import java.nio.ByteBuffer;

public class WBWIRocksIterator
    extends AbstractRocksIterator<WriteBatchWithIndex> {
  private final WriteEntry entry = new WriteEntry();

  protected WBWIRocksIterator(final WriteBatchWithIndex wbwi,
      final long nativeHandle) {
    super(wbwi, nativeHandle);
  }

  /**
   * Get the current entry
   * <p>
   * The WriteEntry is only valid
   * until the iterator is repositioned.
   * If you want to keep the WriteEntry across iterator
   * movements, you must make a copy of its data!
   * <p>
   * Note - This method is not thread-safe with respect to the WriteEntry
   * as it performs a non-atomic update across the fields of the WriteEntry
   *
   * @return The WriteEntry of the current entry
   */
  public WriteEntry entry() {
    assert(isOwningHandle());
    final long[] ptrs = entry1(nativeHandle_);

    entry.type = WriteType.fromId((byte)ptrs[0]);
    entry.key.resetNativeHandle(ptrs[1], ptrs[1] != 0);
    entry.value.resetNativeHandle(ptrs[2], ptrs[2] != 0);

    return entry;
  }

  @Override final native void refresh1(long handle, long snapshotHandle);
  @Override
  protected final void disposeInternal(final long handle) {
    disposeInternalJni(handle);
  }
  private static native void disposeInternalJni(final long handle);
  @Override
  final boolean isValid0(long handle) {
    return isValid0Jni(handle);
  }
  private static native boolean isValid0Jni(long handle);
  @Override
  final void seekToFirst0(long handle) {
    seekToFirst0Jni(handle);
  }
  private static native void seekToFirst0Jni(long handle);
  @Override
  final void seekToLast0(long handle) {
    seekToLast0Jni(handle);
  }
  private static native void seekToLast0Jni(long handle);
  @Override
  final void next0(long handle) {
    next0Jni(handle);
  }
  private static native void next0Jni(long handle);
  @Override
  final void prev0(long handle) {
    prev0Jni(handle);
  }
  private static native void prev0Jni(long handle);
  @Override
  final void refresh0(final long handle) throws RocksDBException {
    refresh0Jni(handle);
  }
  private static native void refresh0Jni(final long handle) throws RocksDBException;
  @Override
  final void seek0(long handle, byte[] target, int targetLen) {
    seek0Jni(handle, target, targetLen);
  }
  private static native void seek0Jni(long handle, byte[] target, int targetLen);
  @Override
  final void seekForPrev0(long handle, byte[] target, int targetLen) {
    seekForPrev0Jni(handle, target, targetLen);
  }
  private static native void seekForPrev0Jni(long handle, byte[] target, int targetLen);
  @Override
  final void status0(long handle) throws RocksDBException {
    status0Jni(handle);
  }
  private static native void status0Jni(long handle) throws RocksDBException;
  @Override
  final void seekDirect0(
      final long handle, final ByteBuffer target, final int targetOffset, final int targetLen) {
    seekDirect0Jni(handle, target, targetOffset, targetLen);
  }
  private static native void seekDirect0Jni(
      final long handle, final ByteBuffer target, final int targetOffset, final int targetLen);
  @Override
  final void seekForPrevDirect0(
      final long handle, final ByteBuffer target, final int targetOffset, final int targetLen) {
    seekForPrevDirect0Jni(handle, target, targetOffset, targetLen);
  }
  private static native void seekForPrevDirect0Jni(
      final long handle, final ByteBuffer target, final int targetOffset, final int targetLen);
  @Override
  final void seekByteArray0(
      final long handle, final byte[] target, final int targetOffset, final int targetLen) {
    seekByteArray0Jni(handle, target, targetOffset, targetLen);
  }
  private static native void seekByteArray0Jni(
      final long handle, final byte[] target, final int targetOffset, final int targetLen);
  @Override
  final void seekForPrevByteArray0(
      final long handle, final byte[] target, final int targetOffset, final int targetLen) {
    seekForPrevByteArray0Jni(handle, target, targetOffset, targetLen);
  }
  private static native void seekForPrevByteArray0Jni(
      final long handle, final byte[] target, final int targetOffset, final int targetLen);

  private static native long[] entry1(final long handle);

  /**
   * Enumeration of the Write operation
   * that created the record in the Write Batch
   */
  public enum WriteType {
    PUT((byte)0x0),
    MERGE((byte)0x1),
    DELETE((byte)0x2),
    SINGLE_DELETE((byte)0x3),
    DELETE_RANGE((byte)0x4),
    LOG((byte)0x5),
    XID((byte)0x6);

    final byte id;
    WriteType(final byte id) {
      this.id = id;
    }

    public static WriteType fromId(final byte id) {
      for(final WriteType wt : WriteType.values()) {
        if(id == wt.id) {
          return wt;
        }
      }
      throw new IllegalArgumentException("No WriteType with id=" + id);
    }
  }

  @Override
  public void close() {
    entry.close();
    super.close();
  }

  /**
   * Represents an entry returned by
   * {@link org.rocksdb.WBWIRocksIterator#entry()}
   *
   * It is worth noting that a WriteEntry with
   * the type {@link org.rocksdb.WBWIRocksIterator.WriteType#DELETE}
   * or {@link org.rocksdb.WBWIRocksIterator.WriteType#LOG}
   * will not have a value.
   */
  public static class WriteEntry implements AutoCloseable {
    WriteType type = null;
    final DirectSlice key;
    final DirectSlice value;

    /**
     * Intentionally private as this
     * should only be instantiated in
     * this manner by the outer WBWIRocksIterator
     * class; The class members are then modified
     * by calling {@link org.rocksdb.WBWIRocksIterator#entry()}
     */
    private WriteEntry() {
      key = new DirectSlice();
      value = new DirectSlice();
    }

    public WriteEntry(final WriteType type, final DirectSlice key,
        final DirectSlice value) {
      this.type = type;
      this.key = key;
      this.value = value;
    }

    /**
     * Returns the type of the Write Entry
     *
     * @return the WriteType of the WriteEntry
     */
    public WriteType getType() {
      return type;
    }

    /**
     * Returns the key of the Write Entry
     *
     * @return The slice containing the key
     * of the WriteEntry
     */
    public DirectSlice getKey() {
      return key;
    }

    /**
     * Returns the value of the Write Entry
     *
     * @return The slice containing the value of
     * the WriteEntry or null if the WriteEntry has
     * no value
     */
    public DirectSlice getValue() {
      if (value.isOwningHandle()) {
        return value;
      } else {
        return null; // TODO(AR) migrate to JDK8 java.util.Optional#empty()
      }
    }

    /**
     * Generates a hash code for the Write Entry. NOTE: The hash code is based
     * on the string representation of the key, so it may not work correctly
     * with exotic custom comparators.
     *
     * @return The hash code for the Write Entry
     */
    @Override
    public int hashCode() {
      return (key == null) ? 0 : key.hashCode();
    }

    @SuppressWarnings("PMD.CloseResource")
    @Override
    public boolean equals(final Object other) {
      if(other == null) {
        return false;
      } else if (this == other) {
        return true;
      } else if(other instanceof WriteEntry) {
        final WriteEntry otherWriteEntry = (WriteEntry)other;
        return type.equals(otherWriteEntry.type)
            && key.equals(otherWriteEntry.key)
            && value.equals(otherWriteEntry.value);
      } else {
        return false;
      }
    }

    @Override
    public void close() {
      value.close();
      key.close();
    }
  }
}
