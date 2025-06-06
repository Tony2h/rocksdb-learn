// Copyright (c) 2011-present, Facebook, Inc.  All rights reserved.
//  This source code is licensed under both the GPLv2 (found in the
//  COPYING file in the root directory) and Apache 2.0 License
//  (found in the LICENSE.Apache file in the root directory).

#include "monitoring/thread_status_updater.h"

#include <memory>

#include "port/likely.h"
#include "rocksdb/env.h"
#include "rocksdb/system_clock.h"
#include "util/mutexlock.h"

namespace ROCKSDB_NAMESPACE {

#ifdef ROCKSDB_USING_THREAD_STATUS

thread_local ThreadStatusData* ThreadStatusUpdater::thread_status_data_ =
    nullptr;

void ThreadStatusUpdater::RegisterThread(ThreadStatus::ThreadType ttype,
                                         uint64_t thread_id) {
  if (UNLIKELY(thread_status_data_ == nullptr)) {
    thread_status_data_ = new ThreadStatusData();
    thread_status_data_->thread_type = ttype;
    thread_status_data_->thread_id = thread_id;
    std::lock_guard<std::mutex> lck(thread_list_mutex_);
    thread_data_set_.insert(thread_status_data_);
  }

  ClearThreadOperationProperties();
}

void ThreadStatusUpdater::UnregisterThread() {
  if (thread_status_data_ != nullptr) {
    std::lock_guard<std::mutex> lck(thread_list_mutex_);
    thread_data_set_.erase(thread_status_data_);
    delete thread_status_data_;
    thread_status_data_ = nullptr;
  }
}

void ThreadStatusUpdater::ResetThreadStatus() {
  ClearThreadState();
  ClearThreadOperation();
  SetColumnFamilyInfoKey(nullptr);
}

void ThreadStatusUpdater::SetEnableTracking(bool enable_tracking) {
  auto* data = Get();
  if (data == nullptr) {
    return;
  }
  data->enable_tracking.store(enable_tracking, std::memory_order_relaxed);
}

void ThreadStatusUpdater::SetColumnFamilyInfoKey(const void* cf_key) {
  auto* data = Get();
  if (data == nullptr) {
    return;
  }
  data->cf_key.store(const_cast<void*>(cf_key), std::memory_order_relaxed);
}

const void* ThreadStatusUpdater::GetColumnFamilyInfoKey() {
  auto* data = GetLocalThreadStatus();
  if (data == nullptr) {
    return nullptr;
  }
  return data->cf_key.load(std::memory_order_relaxed);
}

void ThreadStatusUpdater::SetThreadOperation(
    const ThreadStatus::OperationType type) {
  auto* data = GetLocalThreadStatus();
  if (data == nullptr) {
    return;
  }
  // NOTE: Our practice here is to set all the thread operation properties
  //       and stage before we set thread operation, and thread operation
  //       will be set in std::memory_order_release.  This is to ensure
  //       whenever a thread operation is not OP_UNKNOWN, we will always
  //       have a consistent information on its properties.
  data->operation_type.store(type, std::memory_order_release);
  if (type == ThreadStatus::OP_UNKNOWN) {
    data->operation_stage.store(ThreadStatus::STAGE_UNKNOWN,
                                std::memory_order_relaxed);
    ClearThreadOperationProperties();
  }
}

ThreadStatus::OperationType ThreadStatusUpdater::GetThreadOperation() {
  ThreadStatusData* data = GetLocalThreadStatus();
  if (data == nullptr) {
    return ThreadStatus::OperationType::OP_UNKNOWN;
  }
  return data->operation_type.load(std::memory_order_relaxed);
}

void ThreadStatusUpdater::SetThreadOperationProperty(int i, uint64_t value) {
  auto* data = GetLocalThreadStatus();
  if (data == nullptr) {
    return;
  }
  data->op_properties[i].store(value, std::memory_order_relaxed);
}

void ThreadStatusUpdater::IncreaseThreadOperationProperty(int i,
                                                          uint64_t delta) {
  auto* data = GetLocalThreadStatus();
  if (data == nullptr) {
    return;
  }
  data->op_properties[i].fetch_add(delta, std::memory_order_relaxed);
}

void ThreadStatusUpdater::SetOperationStartTime(const uint64_t start_time) {
  auto* data = GetLocalThreadStatus();
  if (data == nullptr) {
    return;
  }
  data->op_start_time.store(start_time, std::memory_order_relaxed);
}

void ThreadStatusUpdater::ClearThreadOperation() {
  auto* data = GetLocalThreadStatus();
  if (data == nullptr) {
    return;
  }
  data->operation_stage.store(ThreadStatus::STAGE_UNKNOWN,
                              std::memory_order_relaxed);
  data->operation_type.store(ThreadStatus::OP_UNKNOWN,
                             std::memory_order_relaxed);
  ClearThreadOperationProperties();
}

void ThreadStatusUpdater::ClearThreadOperationProperties() {
  auto* data = GetLocalThreadStatus();
  if (data == nullptr) {
    return;
  }
  for (int i = 0; i < ThreadStatus::kNumOperationProperties; ++i) {
    data->op_properties[i].store(0, std::memory_order_relaxed);
  }
}

ThreadStatus::OperationStage ThreadStatusUpdater::SetThreadOperationStage(
    ThreadStatus::OperationStage stage) {
  auto* data = GetLocalThreadStatus();
  if (data == nullptr) {
    return ThreadStatus::STAGE_UNKNOWN;
  }
  return data->operation_stage.exchange(stage, std::memory_order_relaxed);
}

void ThreadStatusUpdater::SetThreadState(const ThreadStatus::StateType type) {
  auto* data = GetLocalThreadStatus();
  if (data == nullptr) {
    return;
  }
  data->state_type.store(type, std::memory_order_relaxed);
}

void ThreadStatusUpdater::ClearThreadState() {
  auto* data = GetLocalThreadStatus();
  if (data == nullptr) {
    return;
  }
  data->state_type.store(ThreadStatus::STATE_UNKNOWN,
                         std::memory_order_relaxed);
}

Status ThreadStatusUpdater::GetThreadList(
    std::vector<ThreadStatus>* thread_list) {
  thread_list->clear();
  std::vector<std::shared_ptr<ThreadStatusData>> valid_list;
  uint64_t now_micros = SystemClock::Default()->NowMicros();

  std::lock_guard<std::mutex> lck(thread_list_mutex_);
  for (auto* thread_data : thread_data_set_) {
    assert(thread_data);
    auto thread_id = thread_data->thread_id.load(std::memory_order_relaxed);
    auto thread_type = thread_data->thread_type.load(std::memory_order_relaxed);
    // Since any change to cf_info_map requires thread_list_mutex,
    // which is currently held by GetThreadList(), here we can safely
    // use "memory_order_relaxed" to load the cf_key.
    auto cf_key = thread_data->cf_key.load(std::memory_order_relaxed);

    ThreadStatus::OperationType op_type = ThreadStatus::OP_UNKNOWN;
    ThreadStatus::OperationStage op_stage = ThreadStatus::STAGE_UNKNOWN;
    ThreadStatus::StateType state_type = ThreadStatus::STATE_UNKNOWN;
    uint64_t op_elapsed_micros = 0;
    uint64_t op_props[ThreadStatus::kNumOperationProperties] = {0};

    auto iter = cf_info_map_.find(cf_key);
    if (iter != cf_info_map_.end()) {
      op_type = thread_data->operation_type.load(std::memory_order_acquire);
      // display lower-level info only when higher-level info is available.
      if (op_type != ThreadStatus::OP_UNKNOWN) {
        op_elapsed_micros = now_micros - thread_data->op_start_time.load(
                                             std::memory_order_relaxed);
        op_stage = thread_data->operation_stage.load(std::memory_order_relaxed);
        state_type = thread_data->state_type.load(std::memory_order_relaxed);
        for (int i = 0; i < ThreadStatus::kNumOperationProperties; ++i) {
          op_props[i] =
              thread_data->op_properties[i].load(std::memory_order_relaxed);
        }
      }
    }

    thread_list->emplace_back(
        thread_id, thread_type,
        iter != cf_info_map_.end() ? iter->second.db_name : "",
        iter != cf_info_map_.end() ? iter->second.cf_name : "", op_type,
        op_elapsed_micros, op_stage, op_props, state_type);
  }

  return Status::OK();
}

ThreadStatusData* ThreadStatusUpdater::GetLocalThreadStatus() {
  if (thread_status_data_ == nullptr) {
    return nullptr;
  }
  if (!thread_status_data_->enable_tracking.load(std::memory_order_relaxed)) {
    return nullptr;
  }
  return thread_status_data_;
}

void ThreadStatusUpdater::NewColumnFamilyInfo(const void* db_key,
                                              const std::string& db_name,
                                              const void* cf_key,
                                              const std::string& cf_name) {
  // Acquiring same lock as GetThreadList() to guarantee
  // a consistent view of global column family table (cf_info_map).
  std::lock_guard<std::mutex> lck(thread_list_mutex_);

  cf_info_map_.emplace(std::piecewise_construct, std::make_tuple(cf_key),
                       std::make_tuple(db_key, db_name, cf_name));
  db_key_map_[db_key].insert(cf_key);
}

void ThreadStatusUpdater::EraseColumnFamilyInfo(const void* cf_key) {
  // Acquiring same lock as GetThreadList() to guarantee
  // a consistent view of global column family table (cf_info_map).
  std::lock_guard<std::mutex> lck(thread_list_mutex_);

  auto cf_pair = cf_info_map_.find(cf_key);
  if (cf_pair != cf_info_map_.end()) {
    // Remove its entry from db_key_map_ by the following steps:
    // 1. Obtain the entry in db_key_map_ whose set contains cf_key
    // 2. Remove it from the set.
    ConstantColumnFamilyInfo& cf_info = cf_pair->second;
    auto db_pair = db_key_map_.find(cf_info.db_key);
    assert(db_pair != db_key_map_.end());
    size_t result __attribute__((__unused__));
    result = db_pair->second.erase(cf_key);
    assert(result);
    cf_info_map_.erase(cf_pair);
  }
}

void ThreadStatusUpdater::EraseDatabaseInfo(const void* db_key) {
  // Acquiring same lock as GetThreadList() to guarantee
  // a consistent view of global column family table (cf_info_map).
  std::lock_guard<std::mutex> lck(thread_list_mutex_);
  auto db_pair = db_key_map_.find(db_key);
  if (UNLIKELY(db_pair == db_key_map_.end())) {
    // In some occasional cases such as DB::Open fails, we won't
    // register ColumnFamilyInfo for a db.
    return;
  }

  for (auto cf_key : db_pair->second) {
    auto cf_pair = cf_info_map_.find(cf_key);
    if (cf_pair != cf_info_map_.end()) {
      cf_info_map_.erase(cf_pair);
    }
  }
  db_key_map_.erase(db_key);
}

#else

void ThreadStatusUpdater::RegisterThread(ThreadStatus::ThreadType /*ttype*/,
                                         uint64_t /*thread_id*/) {}

void ThreadStatusUpdater::UnregisterThread() {}

void ThreadStatusUpdater::ResetThreadStatus() {}

void ThreadStatusUpdater::SetColumnFamilyInfoKey(const void* /*cf_key*/) {}

void ThreadStatusUpdater::SetThreadOperation(
    const ThreadStatus::OperationType /*type*/) {}

void ThreadStatusUpdater::ClearThreadOperation() {}

void ThreadStatusUpdater::SetThreadState(
    const ThreadStatus::StateType /*type*/) {}

void ThreadStatusUpdater::ClearThreadState() {}

Status ThreadStatusUpdater::GetThreadList(
    std::vector<ThreadStatus>* /*thread_list*/) {
  return Status::NotSupported(
      "GetThreadList is not supported in the current running environment.");
}

void ThreadStatusUpdater::NewColumnFamilyInfo(const void* /*db_key*/,
                                              const std::string& /*db_name*/,
                                              const void* /*cf_key*/,
                                              const std::string& /*cf_name*/) {}

void ThreadStatusUpdater::EraseColumnFamilyInfo(const void* /*cf_key*/) {}

void ThreadStatusUpdater::EraseDatabaseInfo(const void* /*db_key*/) {}

void ThreadStatusUpdater::SetThreadOperationProperty(int /*i*/,
                                                     uint64_t /*value*/) {}

void ThreadStatusUpdater::IncreaseThreadOperationProperty(int /*i*/,
                                                          uint64_t /*delta*/) {}

#endif  // ROCKSDB_USING_THREAD_STATUS
}  // namespace ROCKSDB_NAMESPACE
