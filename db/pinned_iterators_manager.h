//  Copyright (c) 2011-present, Facebook, Inc.  All rights reserved.
//  This source code is licensed under both the GPLv2 (found in the
//  COPYING file in the root directory) and Apache 2.0 License
//  (found in the LICENSE.Apache file in the root directory).
//
#pragma once
#include <algorithm>
#include <memory>
#include <utility>
#include <vector>

#include "table/internal_iterator.h"

namespace ROCKSDB_NAMESPACE {

// PinnedIteratorsManager will be notified whenever we need to pin an Iterator
// and it will be responsible for deleting pinned Iterators when they are
// not needed anymore.
class PinnedIteratorsManager : public Cleanable {
 public:
  PinnedIteratorsManager() : pinning_enabled(false) {}
  ~PinnedIteratorsManager() {
    if (pinning_enabled) {
      ReleasePinnedData();
    }
  }

  // Move constructor and move assignment is allowed.
  PinnedIteratorsManager(PinnedIteratorsManager&& other) noexcept = default;
  PinnedIteratorsManager& operator=(PinnedIteratorsManager&& other) noexcept =
      default;

  // Enable Iterators pinning
  void StartPinning() {
    assert(pinning_enabled == false);
    pinning_enabled = true;
  }

  // Is pinning enabled ?
  bool PinningEnabled() { return pinning_enabled; }

  // Take ownership of iter and delete it when ReleasePinnedData() is called
  void PinIterator(InternalIterator* iter, bool arena = false) {
    if (arena) {
      PinPtr(iter, &PinnedIteratorsManager::ReleaseArenaInternalIterator);
    } else {
      PinPtr(iter, &PinnedIteratorsManager::ReleaseInternalIterator);
    }
  }

  using ReleaseFunction = void (*)(void* arg1);
  void PinPtr(void* ptr, ReleaseFunction release_func) {
    assert(pinning_enabled);
    if (ptr == nullptr) {
      return;
    }
    pinned_ptrs_.emplace_back(ptr, release_func);
  }

  // Release pinned Iterators
  inline void ReleasePinnedData() {
    assert(pinning_enabled == true);
    pinning_enabled = false;

    // Remove duplicate pointers
    std::sort(pinned_ptrs_.begin(), pinned_ptrs_.end());
    auto unique_end = std::unique(pinned_ptrs_.begin(), pinned_ptrs_.end());

    for (auto i = pinned_ptrs_.begin(); i != unique_end; ++i) {
      void* ptr = i->first;
      ReleaseFunction release_func = i->second;
      release_func(ptr);
    }
    pinned_ptrs_.clear();
    // Also do cleanups from the base Cleanable
    Cleanable::Reset();
  }

 private:
  static void ReleaseInternalIterator(void* ptr) {
    delete static_cast<InternalIterator*>(ptr);
  }

  static void ReleaseArenaInternalIterator(void* ptr) {
    static_cast<InternalIterator*>(ptr)->~InternalIterator();
  }

  bool pinning_enabled;
  std::vector<std::pair<void*, ReleaseFunction>> pinned_ptrs_;
};

}  // namespace ROCKSDB_NAMESPACE
