// Copyright (c) 2011-present, Facebook, Inc.  All rights reserved.
//  This source code is licensed under both the GPLv2 (found in the
//  COPYING file in the root directory) and Apache 2.0 License
//  (found in the LICENSE.Apache file in the root directory).
//
// This file demonstrates how to use the utility functions defined in
// rocksdb/utilities/options_util.h to open a rocksdb database without
// remembering all the rocksdb options.
#include <cstdio>
#include <string>
#include <vector>

#include "rocksdb/cache.h"
#include "rocksdb/compaction_filter.h"
#include "rocksdb/db.h"
#include "rocksdb/options.h"
#include "rocksdb/slice.h"
#include "rocksdb/table.h"
#include "rocksdb/utilities/options_util.h"

using ROCKSDB_NAMESPACE::BlockBasedTableOptions;
using ROCKSDB_NAMESPACE::ColumnFamilyDescriptor;
using ROCKSDB_NAMESPACE::ColumnFamilyHandle;
using ROCKSDB_NAMESPACE::ColumnFamilyOptions;
using ROCKSDB_NAMESPACE::CompactionFilter;
using ROCKSDB_NAMESPACE::ConfigOptions;
using ROCKSDB_NAMESPACE::DB;
using ROCKSDB_NAMESPACE::DBOptions;
using ROCKSDB_NAMESPACE::NewLRUCache;
using ROCKSDB_NAMESPACE::Options;
using ROCKSDB_NAMESPACE::Slice;
using ROCKSDB_NAMESPACE::Status;

#if defined(OS_WIN)
std::string kDBPath = "C:\\Windows\\TEMP\\rocksdb_options_file_example";
#else
std::string kDBPath = "/tmp/rocksdb_options_file_example";
#endif

namespace {
// A dummy compaction filter
class DummyCompactionFilter : public CompactionFilter {
 public:
  virtual ~DummyCompactionFilter() {}
  virtual bool Filter(int level, const Slice& key, const Slice& existing_value,
                      std::string* new_value, bool* value_changed) const {
    return false;
  }
  virtual const char* Name() const { return "DummyCompactionFilter"; }
};

}  // namespace

int main() {
  DBOptions db_opt;
  db_opt.create_if_missing = true;

  std::vector<ColumnFamilyDescriptor> cf_descs;
  cf_descs.push_back(
      {ROCKSDB_NAMESPACE::kDefaultColumnFamilyName, ColumnFamilyOptions()});
  cf_descs.push_back({"new_cf", ColumnFamilyOptions()});

  // initialize BlockBasedTableOptions
  auto cache = NewLRUCache(1 * 1024 * 1024 * 1024);
  BlockBasedTableOptions bbt_opts;
  bbt_opts.block_size = 32 * 1024;
  bbt_opts.block_cache = cache;

  // initialize column families options
  std::unique_ptr<CompactionFilter> compaction_filter;
  compaction_filter.reset(new DummyCompactionFilter());
  cf_descs[0].options.table_factory.reset(NewBlockBasedTableFactory(bbt_opts));
  cf_descs[0].options.compaction_filter = compaction_filter.get();
  cf_descs[1].options.table_factory.reset(NewBlockBasedTableFactory(bbt_opts));

  // destroy and open DB
  DB* db;
  Status s = ROCKSDB_NAMESPACE::DestroyDB(kDBPath,
                                          Options(db_opt, cf_descs[0].options));
  assert(s.ok());
  s = DB::Open(Options(db_opt, cf_descs[0].options), kDBPath, &db);
  assert(s.ok());

  // Create column family, and rocksdb will persist the options.
  ColumnFamilyHandle* cf;
  s = db->CreateColumnFamily(ColumnFamilyOptions(), "new_cf", &cf);
  assert(s.ok());

  // close DB
  delete cf;
  delete db;

  // In the following code, we will reopen the rocksdb instance using
  // the options file stored in the db directory.

  // Load the options file.
  DBOptions loaded_db_opt;
  std::vector<ColumnFamilyDescriptor> loaded_cf_descs;
  ConfigOptions config_options;
  s = LoadLatestOptions(config_options, kDBPath, &loaded_db_opt,
                        &loaded_cf_descs);
  assert(s.ok());
  assert(loaded_db_opt.create_if_missing == db_opt.create_if_missing);

  // Initialize pointer options for each column family
  for (size_t i = 0; i < loaded_cf_descs.size(); ++i) {
    auto* loaded_bbt_opt =
        loaded_cf_descs[0]
            .options.table_factory->GetOptions<BlockBasedTableOptions>();
    // Expect the same as BlockBasedTableOptions will be loaded form file.
    assert(loaded_bbt_opt->block_size == bbt_opts.block_size);
    // However, block_cache needs to be manually initialized as documented
    // in rocksdb/utilities/options_util.h.
    loaded_bbt_opt->block_cache = cache;
  }
  // In addition, as pointer options are initialized with default value,
  // we need to properly initialized all the pointer options if non-defalut
  // values are used before calling DB::Open().
  assert(loaded_cf_descs[0].options.compaction_filter == nullptr);
  loaded_cf_descs[0].options.compaction_filter = compaction_filter.get();

  // reopen the db using the loaded options.
  std::vector<ColumnFamilyHandle*> handles;
  s = DB::Open(loaded_db_opt, kDBPath, loaded_cf_descs, &handles, &db);
  assert(s.ok());

  // close DB
  for (auto* handle : handles) {
    delete handle;
  }
  delete db;
}
