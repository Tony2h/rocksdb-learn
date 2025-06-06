// Copyright (c) 2011-present, Facebook, Inc.  All rights reserved.
//  This source code is licensed under both the GPLv2 (found in the
//  COPYING file in the root directory) and Apache 2.0 License
//  (found in the LICENSE.Apache file in the root directory).

#pragma once

#include <memory>
#include <vector>

#include "rocksdb/status.h"
#include "rocksdb/types.h"
#include "rocksdb/write_batch.h"

namespace ROCKSDB_NAMESPACE {

class WalFile;
using VectorWalPtr = std::vector<std::unique_ptr<WalFile>>;
// DEPRECATED old name
using VectorLogPtr = VectorWalPtr;

enum WalFileType {
  /* Indicates that WAL file is in archive directory. WAL files are moved from
   * the main db directory to archive directory once they are not live and stay
   * there until cleaned up. Files are cleaned depending on archive size
   * (Options::WAL_size_limit_MB) and time since last cleaning
   * (Options::WAL_ttl_seconds).
   */
  kArchivedLogFile = 0,

  /* Indicates that WAL file is live and resides in the main db directory */
  kAliveLogFile = 1
};

class WalFile {
 public:
  WalFile() {}
  virtual ~WalFile() {}

  // Returns log file's pathname relative to the main db dir
  // Eg. For a live-log-file = /000003.log
  //     For an archived-log-file = /archive/000003.log
  virtual std::string PathName() const = 0;

  // Primary identifier for log file.
  // This is directly proportional to creation time of the log file
  virtual uint64_t LogNumber() const = 0;

  // Log file can be either alive or archived
  virtual WalFileType Type() const = 0;

  // Starting sequence number of writebatch written in this log file
  virtual SequenceNumber StartSequence() const = 0;

  // The position of the last flushed write to the file (which for
  // recycled WAL files is typically less than the full file size).
  virtual uint64_t SizeFileBytes() const = 0;
};

// DEPRECATED old name for WalFile. (Confusing with "Logger" etc.)
using LogFile = WalFile;

struct BatchResult {
  SequenceNumber sequence = 0;
  std::unique_ptr<WriteBatch> writeBatchPtr;

  // Add empty __ctor and __dtor for the rule of five
  // However, preserve the original semantics and prohibit copying
  // as the std::unique_ptr member does not copy.
  BatchResult() {}

  ~BatchResult() {}

  BatchResult(const BatchResult&) = delete;

  BatchResult& operator=(const BatchResult&) = delete;

  BatchResult(BatchResult&& bResult)
      : sequence(std::move(bResult.sequence)),
        writeBatchPtr(std::move(bResult.writeBatchPtr)) {}

  BatchResult& operator=(BatchResult&& bResult) {
    sequence = std::move(bResult.sequence);
    writeBatchPtr = std::move(bResult.writeBatchPtr);
    return *this;
  }
};

// A TransactionLogIterator is used to iterate over the transactions in a db.
// One run of the iterator is continuous, i.e. the iterator will stop at the
// beginning of any gap in sequences
class TransactionLogIterator {
 public:
  TransactionLogIterator() {}
  virtual ~TransactionLogIterator() {}

  // An iterator is either positioned at a WriteBatch or not valid.
  // This method returns true if the iterator is valid.
  // Can read data from a valid iterator.
  virtual bool Valid() = 0;

  // Moves the iterator to the next WriteBatch.
  // REQUIRES: Valid() to be true.
  virtual void Next() = 0;

  // Returns ok if the iterator is valid.
  // Returns the Error when something has gone wrong.
  virtual Status status() = 0;

  // If valid return's the current write_batch and the sequence number of the
  // earliest transaction contained in the batch.
  // ONLY use if Valid() is true and status() is OK.
  virtual BatchResult GetBatch() = 0;

  // The read options for TransactionLogIterator.
  struct ReadOptions {
    // If true, all data read from underlying storage will be
    // verified against corresponding checksums.
    // Default: true
    bool verify_checksums_;

    ReadOptions() : verify_checksums_(true) {}

    explicit ReadOptions(bool verify_checksums)
        : verify_checksums_(verify_checksums) {}
  };
};
}  // namespace ROCKSDB_NAMESPACE
