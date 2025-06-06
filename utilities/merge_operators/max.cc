//  Copyright (c) 2011-present, Facebook, Inc.  All rights reserved.
//  This source code is licensed under both the GPLv2 (found in the
//  COPYING file in the root directory) and Apache 2.0 License
//  (found in the LICENSE.Apache file in the root directory).

#include <memory>

#include "rocksdb/merge_operator.h"
#include "rocksdb/slice.h"
#include "utilities/merge_operators.h"
#include "utilities/merge_operators/max_operator.h"

namespace ROCKSDB_NAMESPACE {

bool MaxOperator::FullMergeV2(const MergeOperationInput& merge_in,
                              MergeOperationOutput* merge_out) const {
  Slice& max = merge_out->existing_operand;
  if (merge_in.existing_value) {
    max =
        Slice(merge_in.existing_value->data(), merge_in.existing_value->size());
  } else if (max.data() == nullptr) {
    max = Slice();
  }

  for (const auto& op : merge_in.operand_list) {
    if (max.compare(op) < 0) {
      max = op;
    }
  }

  return true;
}

bool MaxOperator::PartialMerge(const Slice& /*key*/, const Slice& left_operand,
                               const Slice& right_operand,
                               std::string* new_value,
                               Logger* /*logger*/) const {
  if (left_operand.compare(right_operand) >= 0) {
    new_value->assign(left_operand.data(), left_operand.size());
  } else {
    new_value->assign(right_operand.data(), right_operand.size());
  }
  return true;
}

bool MaxOperator::PartialMergeMulti(const Slice& /*key*/,
                                    const std::deque<Slice>& operand_list,
                                    std::string* new_value,
                                    Logger* /*logger*/) const {
  Slice max;
  for (const auto& operand : operand_list) {
    if (max.compare(operand) < 0) {
      max = operand;
    }
  }

  new_value->assign(max.data(), max.size());
  return true;
}

std::shared_ptr<MergeOperator> MergeOperators::CreateMaxOperator() {
  return std::make_shared<MaxOperator>();
}
}  // namespace ROCKSDB_NAMESPACE
