// Copyright (c) 2011-present, Facebook, Inc.  All rights reserved.
//  This source code is licensed under both the GPLv2 (found in the
//  COPYING file in the root directory) and Apache 2.0 License
//  (found in the LICENSE.Apache file in the root directory).

#pragma once

#include <deque>
#include <memory>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include "rocksdb/customizable.h"
#include "rocksdb/slice.h"
#include "rocksdb/wide_columns.h"

namespace ROCKSDB_NAMESPACE {

class Slice;
class Logger;

// The Merge Operator
//
// Essentially, a MergeOperator specifies the SEMANTICS of a merge, which only
// client knows. It could be numeric addition, list append, string
// concatenation, edit data structure, ... , anything.
// The library, on the other hand, is concerned with the exercise of this
// interface, at the right time (during get, iteration, compaction...)
//
// To use merge, the client needs to provide an object implementing one of
// the following interfaces:
//  a) AssociativeMergeOperator - for most simple semantics (always take
//    two values, and merge them into one value, which is then put back
//    into rocksdb); numeric addition and string concatenation are examples;
//
//  b) MergeOperator - the generic class for all the more abstract / complex
//    operations; one method (FullMergeV3) to merge a Put/Delete value with a
//    merge operand; and another method (PartialMerge) that merges multiple
//    operands together. this is especially useful if your key values have
//    complex structures but you would still like to support client-specific
//    incremental updates.
//
// AssociativeMergeOperator is simpler to implement. MergeOperator is simply
// more powerful.
//
// Refer to rocksdb-merge wiki for more details and example implementations.
//
// Exceptions MUST NOT propagate out of overridden functions into RocksDB,
// because RocksDB is not exception-safe. This could cause undefined behavior
// including data loss, unreported corruption, deadlocks, and more.
class MergeOperator : public Customizable {
 public:
  virtual ~MergeOperator() {}
  static const char* Type() { return "MergeOperator"; }
  static Status CreateFromString(const ConfigOptions& opts,
                                 const std::string& id,
                                 std::shared_ptr<MergeOperator>* result);

  // Gives the client a way to express the read -> modify -> write semantics
  // key:      (IN)    The key that's associated with this merge operation.
  //                   Client could multiplex the merge operator based on it
  //                   if the key space is partitioned and different subspaces
  //                   refer to different types of data which have different
  //                   merge operation semantics
  // existing: (IN)    null indicates that the key does not exist before this op
  // operand_list:(IN) the sequence of merge operations to apply, front() first.
  // new_value:(OUT)   Client is responsible for filling the merge result here.
  // The string that new_value is pointing to will be empty.
  // logger:   (IN)    Client could use this to log errors during merge.
  //
  // Return true on success.
  // All values passed in will be client-specific values. So if this method
  // returns false, it is because client specified bad data or there was
  // internal corruption. This will be treated as an error by the library.
  //
  // Also make use of the *logger for error messages.
  virtual bool FullMerge(const Slice& /*key*/, const Slice* /*existing_value*/,
                         const std::deque<std::string>& /*operand_list*/,
                         std::string* /*new_value*/, Logger* /*logger*/) const {
    // deprecated, please use FullMergeV2()
    assert(false);
    return false;
  }

  struct MergeOperationInput {
    // If user-defined timestamp is enabled, `_key` includes timestamp.
    explicit MergeOperationInput(const Slice& _key,
                                 const Slice* _existing_value,
                                 const std::vector<Slice>& _operand_list,
                                 Logger* _logger)
        : key(_key),
          existing_value(_existing_value),
          operand_list(_operand_list),
          logger(_logger) {}

    // The key associated with the merge operation.
    const Slice& key;
    // The existing value of the current key, nullptr means that the
    // value doesn't exist.
    const Slice* existing_value;
    // A list of operands to apply.
    const std::vector<Slice>& operand_list;
    // Logger could be used by client to log any errors that happen during
    // the merge operation.
    Logger* logger;
  };

  enum class OpFailureScope {
    kDefault,
    kTryMerge,
    kMustMerge,
    kOpFailureScopeMax,
  };

  struct MergeOperationOutput {
    explicit MergeOperationOutput(std::string& _new_value,
                                  Slice& _existing_operand)
        : new_value(_new_value), existing_operand(_existing_operand) {}

    // Client is responsible for filling the merge result here.
    std::string& new_value;
    // If the merge result is one of the existing operands (or existing_value),
    // client can set this field to the operand (or existing_value) instead of
    // using new_value.
    Slice& existing_operand;
    // Indicates the blast radius of the failure. It is only meaningful to
    // provide a failure scope when returning `false` from the API populating
    // the `MergeOperationOutput`. Currently RocksDB operations handle these
    // values as follows:
    //
    // - `OpFailureScope::kDefault`: fallback to default
    //   (`OpFailureScope::kTryMerge`)
    // - `OpFailureScope::kTryMerge`: operations that try to merge that key will
    //   fail. This includes flush and compaction, which puts the DB in
    //   read-only mode.
    // - `OpFailureScope::kMustMerge`: operations that must merge that key will
    //   fail (e.g., `Get()`, `MultiGet()`, iteration). Flushes/compactions can
    //   still proceed by copying the original input operands to the output.
    OpFailureScope op_failure_scope = OpFailureScope::kDefault;
  };

  // This function applies a stack of merge operands in chronological order
  // on top of an existing value. There are two ways in which this method is
  // being used:
  // a) During Get() operation, it used to calculate the final value of a key
  // b) During compaction, in order to collapse some operands with the based
  //    value.
  //
  // Note: The name of the method is somewhat misleading, as both in the cases
  // of Get() or compaction it may be called on a subset of operands:
  // K:    0    +1    +2    +7    +4     +5      2     +1     +2
  //                              ^
  //                              |
  //                          snapshot
  // In the example above, Get(K) operation will call FullMerge with a base
  // value of 2 and operands [+1, +2]. Compaction process might decide to
  // collapse the beginning of the history up to the snapshot by performing
  // full Merge with base value of 0 and operands [+1, +2, +7, +4].
  virtual bool FullMergeV2(const MergeOperationInput& merge_in,
                           MergeOperationOutput* merge_out) const;

  struct MergeOperationInputV3 {
    using ExistingValue = std::variant<std::monostate, Slice, WideColumns>;
    using OperandList = std::vector<Slice>;

    explicit MergeOperationInputV3(const Slice& _key,
                                   ExistingValue&& _existing_value,
                                   const OperandList& _operand_list,
                                   Logger* _logger)
        : key(_key),
          existing_value(std::move(_existing_value)),
          operand_list(_operand_list),
          logger(_logger) {}

    // The user key, including the user-defined timestamp if applicable.
    const Slice& key;
    // The base value of the merge operation. Can be one of three things (see
    // the ExistingValue variant above): no existing value, plain existing
    // value, or wide-column existing value.
    ExistingValue existing_value;
    // The list of operands to apply.
    const OperandList& operand_list;
    // The logger to use in case a failure happens during the merge operation.
    Logger* logger;
  };

  struct MergeOperationOutputV3 {
    using NewColumns = std::vector<std::pair<std::string, std::string>>;
    using NewValue = std::variant<std::string, NewColumns, Slice>;

    // The result of the merge operation. Can be one of three things (see the
    // NewValue variant above): a new plain value, a new wide-column value, or
    // an existing merge operand.
    NewValue new_value;
    // The scope of the failure if applicable. See above for more details.
    OpFailureScope op_failure_scope = OpFailureScope::kDefault;
  };

  // An extended version of FullMergeV2() that supports wide columns on both the
  // input and the output side, enabling the application to perform general
  // transformations during merges. For backward compatibility, the default
  // implementation calls FullMergeV2(). Specifically, if there is no base value
  // or the base value is a plain key-value, the default implementation falls
  // back to FullMergeV2(). If the base value is a wide-column entity, the
  // default implementation invokes FullMergeV2() to perform the merge on the
  // default column, and leaves any other columns unchanged.
  virtual bool FullMergeV3(const MergeOperationInputV3& merge_in,
                           MergeOperationOutputV3* merge_out) const;

  // This function performs merge(left_op, right_op)
  // when both the operands are themselves merge operation types
  // that you would have passed to a DB::Merge() call in the same order
  // (i.e.: DB::Merge(key,left_op), followed by DB::Merge(key,right_op)).
  //
  // PartialMerge should combine them into a single merge operation that is
  // saved into *new_value, and then it should return true.
  // *new_value should be constructed such that a call to
  // DB::Merge(key, *new_value) would yield the same result as a call
  // to DB::Merge(key, left_op) followed by DB::Merge(key, right_op).
  //
  // The string that new_value is pointing to will be empty.
  //
  // The default implementation of PartialMergeMulti will use this function
  // as a helper, for backward compatibility.  Any successor class of
  // MergeOperator should either implement PartialMerge or PartialMergeMulti,
  // although implementing PartialMergeMulti is suggested as it is in general
  // more effective to merge multiple operands at a time instead of two
  // operands at a time.
  //
  // If it is impossible or infeasible to combine the two operations,
  // leave new_value unchanged and return false. The library will
  // internally keep track of the operations, and apply them in the
  // correct order once a base-value (a Put/Delete/End-of-Database) is seen.
  //
  // TODO: Presently there is no way to differentiate between error/corruption
  // and simply "return false". For now, the client should simply return
  // false in any case it cannot perform partial-merge, regardless of reason.
  // If there is corruption in the data, handle it in the FullMergeV3() function
  // and return false there.  The default implementation of PartialMerge will
  // always return false.
  virtual bool PartialMerge(const Slice& /*key*/, const Slice& /*left_operand*/,
                            const Slice& /*right_operand*/,
                            std::string* /*new_value*/,
                            Logger* /*logger*/) const {
    return false;
  }

  // This function performs merge when all the operands are themselves merge
  // operation types that you would have passed to a DB::Merge() call in the
  // same order (front() first)
  // (i.e. DB::Merge(key, operand_list[0]), followed by
  //  DB::Merge(key, operand_list[1]), ...)
  //
  // PartialMergeMulti should combine them into a single merge operation that is
  // saved into *new_value, and then it should return true.  *new_value should
  // be constructed such that a call to DB::Merge(key, *new_value) would yield
  // the same result as sequential individual calls to DB::Merge(key, operand)
  // for each operand in operand_list from front() to back().
  //
  // The string that new_value is pointing to will be empty.
  //
  // The PartialMergeMulti function will be called when there are at least two
  // operands.
  //
  // In the default implementation, PartialMergeMulti will invoke PartialMerge
  // multiple times, where each time it only merges two operands.  Developers
  // should either implement PartialMergeMulti, or implement PartialMerge which
  // is served as the helper function of the default PartialMergeMulti.
  virtual bool PartialMergeMulti(const Slice& key,
                                 const std::deque<Slice>& operand_list,
                                 std::string* new_value, Logger* logger) const;

  // The name of the MergeOperator. Used to check for MergeOperator
  // mismatches (i.e., a DB created with one MergeOperator is
  // accessed using a different MergeOperator)
  // TODO: the name is currently not stored persistently and thus
  //       no checking is enforced. Client is responsible for providing
  //       consistent MergeOperator between DB opens.
  const char* Name() const override = 0;

  // Determines whether the PartialMerge can be called with just a single
  // merge operand.
  // Override and return true for allowing a single operand. PartialMerge
  // and PartialMergeMulti should be overridden and implemented
  // correctly to properly handle a single operand.
  virtual bool AllowSingleOperand() const { return false; }

  // Allows to control when to invoke a full merge during Get.
  // This could be used to limit the number of merge operands that are looked at
  // during a point lookup, thereby helping in limiting the number of levels to
  // read from.
  // Doesn't help with iterators.
  //
  // Note: the merge operands are passed to this function in the reversed order
  // relative to how they were merged (passed to
  // FullMerge/FullMergeV2/FullMergeV3) for performance reasons, see also:
  // https://github.com/facebook/rocksdb/issues/3865
  virtual bool ShouldMerge(const std::vector<Slice>& /*operands*/) const {
    return false;
  }
};

// The simpler, associative merge operator.
class AssociativeMergeOperator : public MergeOperator {
 public:
  ~AssociativeMergeOperator() override {}

  // Gives the client a way to express the read -> modify -> write semantics
  // key:           (IN) The key that's associated with this merge operation.
  // existing_value:(IN) null indicates the key does not exist before this op
  // value:         (IN) the value to update/merge the existing_value with
  // new_value:    (OUT) Client is responsible for filling the merge result
  // here. The string that new_value is pointing to will be empty.
  // logger:        (IN) Client could use this to log errors during merge.
  //
  // Return true on success.
  // All values passed in will be client-specific values. So if this method
  // returns false, it is because client specified bad data or there was
  // internal corruption. The client should assume that this will be treated
  // as an error by the library.
  virtual bool Merge(const Slice& key, const Slice* existing_value,
                     const Slice& value, std::string* new_value,
                     Logger* logger) const = 0;

 private:
  // Default implementations of the MergeOperator functions
  bool FullMergeV2(const MergeOperationInput& merge_in,
                   MergeOperationOutput* merge_out) const override;

  bool PartialMerge(const Slice& key, const Slice& left_operand,
                    const Slice& right_operand, std::string* new_value,
                    Logger* logger) const override;
};

}  // namespace ROCKSDB_NAMESPACE
