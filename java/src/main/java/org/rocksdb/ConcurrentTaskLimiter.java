//  Copyright (c) Meta Platforms, Inc. and affiliates.
//
//  This source code is licensed under both the GPLv2 (found in the
//  COPYING file in the root directory) and Apache 2.0 License
//  (found in the LICENSE.Apache file in the root directory).

package org.rocksdb;

public abstract class ConcurrentTaskLimiter extends RocksObject {
  protected ConcurrentTaskLimiter(final long nativeHandle) {
    super(nativeHandle);
  }

  /**
   * Returns a name that identifies this concurrent task limiter.
   *
   * @return Concurrent task limiter name.
   */
  public abstract String name();

  /**
   * Set max concurrent tasks.<br>
   * limit = 0 means no new task allowed.<br>
   * limit &lt; 0 means no limitation.
   *
   * @param maxOutstandinsTask max concurrent tasks.
   * @return the reference to the current instance of ConcurrentTaskLimiter.
   */
  public abstract ConcurrentTaskLimiter setMaxOutstandingTask(final int maxOutstandinsTask);

  /**
   * Reset to unlimited max concurrent task.
   *
   * @return the reference to the current instance of ConcurrentTaskLimiter.
   */
  public abstract ConcurrentTaskLimiter resetMaxOutstandingTask();

  /**
   * Returns current outstanding task count.
   *
   * @return current outstanding task count.
   */
  public abstract int outstandingTask();
}
