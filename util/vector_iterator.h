// Copyright (c) Facebook, Inc. and its affiliates. All Rights Reserved.
#pragma once

#include <algorithm>
#include <string>
#include <vector>

#include "db/dbformat.h"
#include "rocksdb/comparator.h"
#include "rocksdb/iterator.h"
#include "rocksdb/slice.h"
#include "table/internal_iterator.h"

namespace ROCKSDB_NAMESPACE {

// Iterator over a vector of keys/values
class VectorIterator : public InternalIterator {
 public:
  VectorIterator(std::vector<std::string> keys, std::vector<std::string> values,
                 const CompareInterface* icmp = nullptr)
      : keys_(std::move(keys)),
        values_(std::move(values)),
        current_(keys_.size()),
        indexed_cmp_(icmp, &keys_) {
    assert(keys_.size() == values_.size());

    indices_.reserve(keys_.size());
    for (size_t i = 0; i < keys_.size(); i++) {
      indices_.push_back(i);
    }
    if (icmp != nullptr) {
      std::sort(indices_.begin(), indices_.end(), indexed_cmp_);
    }
  }

  bool Valid() const override {
    return !indices_.empty() && current_ < indices_.size();
  }

  void SeekToFirst() override { current_ = 0; }
  void SeekToLast() override { current_ = indices_.size() - 1; }

  void Seek(const Slice& target) override {
    if (indexed_cmp_.cmp != nullptr) {
      current_ = std::lower_bound(indices_.begin(), indices_.end(), target,
                                  indexed_cmp_) -
                 indices_.begin();
    } else {
      current_ =
          std::lower_bound(keys_.begin(), keys_.end(), target.ToString()) -
          keys_.begin();
    }
  }

  void SeekForPrev(const Slice& target) override {
    if (indexed_cmp_.cmp != nullptr) {
      current_ = std::upper_bound(indices_.begin(), indices_.end(), target,
                                  indexed_cmp_) -
                 indices_.begin();
    } else {
      current_ =
          std::upper_bound(keys_.begin(), keys_.end(), target.ToString()) -
          keys_.begin();
    }
    if (!Valid()) {
      SeekToLast();
    } else {
      Prev();
    }
  }

  void Next() override { current_++; }
  void Prev() override { current_--; }

  Slice key() const override { return Slice(keys_[indices_[current_]]); }
  Slice value() const override { return Slice(values_[indices_[current_]]); }

  Status status() const override { return Status::OK(); }

  bool IsKeyPinned() const override { return true; }
  bool IsValuePinned() const override { return true; }

 protected:
  std::vector<std::string> keys_;
  std::vector<std::string> values_;
  size_t current_;

 private:
  struct IndexedKeyComparator {
    IndexedKeyComparator(const CompareInterface* c,
                         const std::vector<std::string>* ks)
        : cmp(c), keys(ks) {}

    bool operator()(size_t a, size_t b) const {
      return cmp->Compare((*keys)[a], (*keys)[b]) < 0;
    }

    bool operator()(size_t a, const Slice& b) const {
      return cmp->Compare((*keys)[a], b) < 0;
    }

    bool operator()(const Slice& a, size_t b) const {
      return cmp->Compare(a, (*keys)[b]) < 0;
    }

    const CompareInterface* cmp;
    const std::vector<std::string>* keys;
  };

  IndexedKeyComparator indexed_cmp_;
  std::vector<size_t> indices_;
};

}  // namespace ROCKSDB_NAMESPACE
