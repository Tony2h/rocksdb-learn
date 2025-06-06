//  Copyright (c) Meta Platforms, Inc. and affiliates.
//
//  This source code is licensed under both the GPLv2 (found in the
//  COPYING file in the root directory) and Apache 2.0 License
//  (found in the LICENSE.Apache file in the root directory).

package org.rocksdb;

public class ConcurrentTaskLimiterImpl extends ConcurrentTaskLimiter {
  public ConcurrentTaskLimiterImpl(final String name, final int maxOutstandingTask) {
    super(newConcurrentTaskLimiterImpl0(name, maxOutstandingTask));
  }

  @Override
  public String name() {
    assert (isOwningHandle());
    return name(nativeHandle_);
  }

  @Override
  public ConcurrentTaskLimiter setMaxOutstandingTask(final int maxOutstandingTask) {
    assert (isOwningHandle());
    setMaxOutstandingTask(nativeHandle_, maxOutstandingTask);
    return this;
  }

  @Override
  public ConcurrentTaskLimiter resetMaxOutstandingTask() {
    assert (isOwningHandle());
    resetMaxOutstandingTask(nativeHandle_);
    return this;
  }

  @Override
  public int outstandingTask() {
    assert (isOwningHandle());
    return outstandingTask(nativeHandle_);
  }

  private static native long newConcurrentTaskLimiterImpl0(
      final String name, final int maxOutstandingTask);
  private static native String name(final long handle);
  private static native void setMaxOutstandingTask(final long handle, final int limit);
  private static native void resetMaxOutstandingTask(final long handle);
  private static native int outstandingTask(final long handle);

  @Override
  protected final void disposeInternal(final long handle) {
    disposeInternalJni(handle);
  }
  private static native void disposeInternalJni(final long handle);
}
