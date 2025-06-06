//  Copyright (c) 2011-present, Facebook, Inc.  All rights reserved.
//  This source code is licensed under both the GPLv2 (found in the
//  COPYING file in the root directory) and Apache 2.0 License
//  (found in the LICENSE.Apache file in the root directory).

#include "rocksdb/utilities/sim_cache.h"

#include <cstdlib>

#include "db/db_test_util.h"
#include "port/stack_trace.h"

namespace ROCKSDB_NAMESPACE {

class SimCacheTest : public DBTestBase {
 private:
  size_t miss_count_ = 0;
  size_t hit_count_ = 0;
  size_t insert_count_ = 0;
  size_t failure_count_ = 0;

 public:
  const size_t kNumBlocks = 5;
  const size_t kValueSize = 1000;

  SimCacheTest() : DBTestBase("sim_cache_test", /*env_do_fsync=*/true) {}

  BlockBasedTableOptions GetTableOptions() {
    BlockBasedTableOptions table_options;
    // Set a small enough block size so that each key-value get its own block.
    table_options.block_size = 1;
    return table_options;
  }

  Options GetOptions(const BlockBasedTableOptions& table_options) {
    Options options = CurrentOptions();
    options.create_if_missing = true;
    // options.compression = kNoCompression;
    options.statistics = ROCKSDB_NAMESPACE::CreateDBStatistics();
    options.table_factory.reset(NewBlockBasedTableFactory(table_options));
    return options;
  }

  void InitTable(const Options& /*options*/) {
    std::string value(kValueSize, 'a');
    for (size_t i = 0; i < kNumBlocks * 2; i++) {
      ASSERT_OK(Put(std::to_string(i), value.c_str()));
    }
  }

  void RecordCacheCounters(const Options& options) {
    miss_count_ = TestGetTickerCount(options, BLOCK_CACHE_MISS);
    hit_count_ = TestGetTickerCount(options, BLOCK_CACHE_HIT);
    insert_count_ = TestGetTickerCount(options, BLOCK_CACHE_ADD);
    failure_count_ = TestGetTickerCount(options, BLOCK_CACHE_ADD_FAILURES);
  }

  void CheckCacheCounters(const Options& options, size_t expected_misses,
                          size_t expected_hits, size_t expected_inserts,
                          size_t expected_failures) {
    size_t new_miss_count = TestGetTickerCount(options, BLOCK_CACHE_MISS);
    size_t new_hit_count = TestGetTickerCount(options, BLOCK_CACHE_HIT);
    size_t new_insert_count = TestGetTickerCount(options, BLOCK_CACHE_ADD);
    size_t new_failure_count =
        TestGetTickerCount(options, BLOCK_CACHE_ADD_FAILURES);
    ASSERT_EQ(miss_count_ + expected_misses, new_miss_count);
    ASSERT_EQ(hit_count_ + expected_hits, new_hit_count);
    ASSERT_EQ(insert_count_ + expected_inserts, new_insert_count);
    ASSERT_EQ(failure_count_ + expected_failures, new_failure_count);
    miss_count_ = new_miss_count;
    hit_count_ = new_hit_count;
    insert_count_ = new_insert_count;
    failure_count_ = new_failure_count;
  }
};

TEST_F(SimCacheTest, SimCache) {
  ReadOptions read_options;
  auto table_options = GetTableOptions();
  auto options = GetOptions(table_options);
  InitTable(options);
  LRUCacheOptions co;
  co.capacity = 0;
  co.num_shard_bits = 0;
  co.strict_capacity_limit = false;
  co.metadata_charge_policy = kDontChargeCacheMetadata;
  std::shared_ptr<SimCache> simCache = NewSimCache(NewLRUCache(co), 20000, 0);
  table_options.block_cache = simCache;
  options.table_factory.reset(NewBlockBasedTableFactory(table_options));
  Reopen(options);
  RecordCacheCounters(options);
  // due to cache entry stats collector
  uint64_t base_misses = simCache->get_miss_counter();

  std::vector<std::unique_ptr<Iterator>> iterators(kNumBlocks);
  Iterator* iter = nullptr;

  // Load blocks into cache.
  for (size_t i = 0; i < kNumBlocks; i++) {
    iter = db_->NewIterator(read_options);
    iter->Seek(std::to_string(i));
    ASSERT_OK(iter->status());
    CheckCacheCounters(options, 1, 0, 1, 0);
    iterators[i].reset(iter);
  }
  ASSERT_EQ(kNumBlocks, simCache->get_hit_counter() +
                            simCache->get_miss_counter() - base_misses);
  ASSERT_EQ(0, simCache->get_hit_counter());
  size_t usage = simCache->GetUsage();
  ASSERT_LT(0, usage);
  ASSERT_EQ(usage, simCache->GetSimUsage());
  simCache->SetCapacity(usage);
  ASSERT_EQ(usage, simCache->GetPinnedUsage());

  // Test with strict capacity limit.
  simCache->SetStrictCapacityLimit(true);
  iter = db_->NewIterator(read_options);
  iter->Seek(std::to_string(kNumBlocks * 2 - 1));
  ASSERT_TRUE(iter->status().IsMemoryLimit());
  CheckCacheCounters(options, 1, 0, 0, 1);
  delete iter;
  iter = nullptr;

  // Release iterators and access cache again.
  for (size_t i = 0; i < kNumBlocks; i++) {
    iterators[i].reset();
    CheckCacheCounters(options, 0, 0, 0, 0);
  }
  // Add kNumBlocks again
  for (size_t i = 0; i < kNumBlocks; i++) {
    std::unique_ptr<Iterator> it(db_->NewIterator(read_options));
    it->Seek(std::to_string(i));
    ASSERT_OK(it->status());
    CheckCacheCounters(options, 0, 1, 0, 0);
  }
  ASSERT_EQ(5, simCache->get_hit_counter());
  for (size_t i = kNumBlocks; i < kNumBlocks * 2; i++) {
    std::unique_ptr<Iterator> it(db_->NewIterator(read_options));
    it->Seek(std::to_string(i));
    ASSERT_OK(it->status());
    CheckCacheCounters(options, 1, 0, 1, 0);
  }
  ASSERT_EQ(0, simCache->GetPinnedUsage());
  ASSERT_EQ(3 * kNumBlocks + 1, simCache->get_hit_counter() +
                                    simCache->get_miss_counter() - base_misses);
  ASSERT_EQ(6, simCache->get_hit_counter());
}

TEST_F(SimCacheTest, SimCacheLogging) {
  auto table_options = GetTableOptions();
  auto options = GetOptions(table_options);
  options.disable_auto_compactions = true;
  LRUCacheOptions co;
  co.capacity = 1024 * 1024;
  co.metadata_charge_policy = kDontChargeCacheMetadata;
  std::shared_ptr<SimCache> sim_cache = NewSimCache(NewLRUCache(co), 20000, 0);
  table_options.block_cache = sim_cache;
  options.table_factory.reset(NewBlockBasedTableFactory(table_options));
  Reopen(options);

  int num_block_entries = 20;
  for (int i = 0; i < num_block_entries; i++) {
    ASSERT_OK(Put(Key(i), "val"));
    ASSERT_OK(Flush());
  }

  std::string log_file = test::PerThreadDBPath(env_, "cache_log.txt");
  ASSERT_OK(sim_cache->StartActivityLogging(log_file, env_));
  for (int i = 0; i < num_block_entries; i++) {
    ASSERT_EQ(Get(Key(i)), "val");
  }
  for (int i = 0; i < num_block_entries; i++) {
    ASSERT_EQ(Get(Key(i)), "val");
  }
  sim_cache->StopActivityLogging();
  ASSERT_OK(sim_cache->GetActivityLoggingStatus());

  std::string file_contents;
  ASSERT_OK(ReadFileToString(env_, log_file, &file_contents));
  std::istringstream contents(file_contents);

  int lookup_num = 0;
  int add_num = 0;

  std::string line;
  // count number of lookups and additions
  while (std::getline(contents, line)) {
    // check if the line starts with LOOKUP or ADD
    if (line.rfind("LOOKUP -", 0) == 0) {
      ++lookup_num;
    }
    if (line.rfind("ADD -", 0) == 0) {
      ++add_num;
    }
  }

  // We asked for every block twice
  ASSERT_EQ(lookup_num, num_block_entries * 2);

  // We added every block only once, since the cache can hold all blocks
  ASSERT_EQ(add_num, num_block_entries);

  // Log things again but stop logging automatically after reaching 512 bytes
  int max_size = 512;
  ASSERT_OK(sim_cache->StartActivityLogging(log_file, env_, max_size));
  for (int it = 0; it < 10; it++) {
    for (int i = 0; i < num_block_entries; i++) {
      ASSERT_EQ(Get(Key(i)), "val");
    }
  }
  ASSERT_OK(sim_cache->GetActivityLoggingStatus());

  uint64_t fsize = 0;
  ASSERT_OK(env_->GetFileSize(log_file, &fsize));
  // error margin of 100 bytes
  ASSERT_LT(fsize, max_size + 100);
  ASSERT_GT(fsize, max_size - 100);
}

}  // namespace ROCKSDB_NAMESPACE

int main(int argc, char** argv) {
  ROCKSDB_NAMESPACE::port::InstallStackTraceHandler();
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
