// Copyright (c) 2011-present, Facebook, Inc. All rights reserved.
//  This source code is licensed under both the GPLv2 (found in the
//  COPYING file in the root directory) and Apache 2.0 License
//  (found in the LICENSE.Apache file in the root directory).

#pragma once

#include <ctime>

#include "memory/arena.h"
#include "port/sys_time.h"
#include "rocksdb/env.h"
#include "util/autovector.h"

namespace ROCKSDB_NAMESPACE {

class Logger;

// A class to buffer info log entries and flush them in the end.
class LogBuffer {
 public:
  // log_level: the log level for all the logs
  // info_log:  logger to write the logs to
  LogBuffer(const InfoLogLevel log_level, Logger* info_log);

  // Add a log entry to the buffer. Use default max_log_size.
  // max_log_size indicates maximize log size, including some metadata.
  void AddLogToBuffer(size_t max_log_size, const char* format, va_list ap);

  size_t IsEmpty() const { return logs_.empty(); }

  // Flush all buffered log to the info log.
  void FlushBufferToLog();
  static const size_t kDefaultMaxLogSize = 512;

 private:
  // One log entry with its timestamp
  struct BufferedLog {
    port::TimeVal now_tv;  // Timestamp of the log
    char message[1];       // Beginning of log message
  };

  const InfoLogLevel log_level_;
  Logger* info_log_;
  Arena arena_;
  autovector<BufferedLog*> logs_;
};

// Add log to the LogBuffer for a delayed info logging. It can be used when
// we want to add some logs inside a mutex.
// max_log_size indicates maximize log size, including some metadata.
void LogToBuffer(LogBuffer* log_buffer, size_t max_log_size, const char* format,
                 ...);
// Same as previous function, but with default max log size.
void LogToBuffer(LogBuffer* log_buffer, const char* format, ...);

}  // namespace ROCKSDB_NAMESPACE
