//  Copyright (c) 2011-present, Facebook, Inc.  All rights reserved.
//  This source code is licensed under both the GPLv2 (found in the
//  COPYING file in the root directory) and Apache 2.0 License
//  (found in the LICENSE.Apache file in the root directory).
//
// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#include "rocksdb/slice_transform.h"

#include "rocksdb/db.h"
#include "rocksdb/env.h"
#include "rocksdb/filter_policy.h"
#include "rocksdb/statistics.h"
#include "rocksdb/table.h"
#include "test_util/testharness.h"

namespace ROCKSDB_NAMESPACE {

class SliceTransformTest : public testing::Test {};

TEST_F(SliceTransformTest, CapPrefixTransform) {
  std::string s;
  s = "abcdefge";

  std::unique_ptr<const SliceTransform> transform;

  transform.reset(NewCappedPrefixTransform(6));
  ASSERT_EQ(transform->Transform(s).ToString(), "abcdef");
  ASSERT_TRUE(transform->SameResultWhenAppended("123456"));
  ASSERT_TRUE(transform->SameResultWhenAppended("1234567"));
  ASSERT_TRUE(!transform->SameResultWhenAppended("12345"));

  transform.reset(NewCappedPrefixTransform(8));
  ASSERT_EQ(transform->Transform(s).ToString(), "abcdefge");

  transform.reset(NewCappedPrefixTransform(10));
  ASSERT_EQ(transform->Transform(s).ToString(), "abcdefge");

  transform.reset(NewCappedPrefixTransform(0));
  ASSERT_EQ(transform->Transform(s).ToString(), "");

  transform.reset(NewCappedPrefixTransform(0));
  ASSERT_EQ(transform->Transform("").ToString(), "");
}

class SliceTransformDBTest : public testing::Test {
 private:
  std::string dbname_;
  Env* env_;
  DB* db_;

 public:
  SliceTransformDBTest() : env_(Env::Default()), db_(nullptr) {
    dbname_ = test::PerThreadDBPath("slice_transform_db_test");
    EXPECT_OK(DestroyDB(dbname_, last_options_));
  }

  ~SliceTransformDBTest() override {
    delete db_;
    EXPECT_OK(DestroyDB(dbname_, last_options_));
  }

  DB* db() { return db_; }

  // Return the current option configuration.
  Options* GetOptions() { return &last_options_; }

  void DestroyAndReopen() {
    // Destroy using last options
    Destroy();
    ASSERT_OK(TryReopen());
  }

  void Destroy() {
    delete db_;
    db_ = nullptr;
    ASSERT_OK(DestroyDB(dbname_, last_options_));
  }

  Status TryReopen() {
    delete db_;
    db_ = nullptr;
    last_options_.create_if_missing = true;

    return DB::Open(last_options_, dbname_, &db_);
  }

  Options last_options_;
};

namespace {
uint64_t PopTicker(const Options& options, Tickers ticker_type) {
  return options.statistics->getAndResetTickerCount(ticker_type);
}
}  // namespace

TEST_F(SliceTransformDBTest, CapPrefix) {
  last_options_.prefix_extractor.reset(NewCappedPrefixTransform(8));
  last_options_.statistics = ROCKSDB_NAMESPACE::CreateDBStatistics();
  BlockBasedTableOptions bbto;
  bbto.filter_policy.reset(NewBloomFilterPolicy(10, false));
  bbto.whole_key_filtering = false;
  last_options_.table_factory.reset(NewBlockBasedTableFactory(bbto));
  ASSERT_OK(TryReopen());

  ReadOptions ro;
  FlushOptions fo;
  WriteOptions wo;

  ASSERT_OK(db()->Put(wo, "barbarbar", "foo"));
  ASSERT_OK(db()->Put(wo, "barbarbar2", "foo2"));
  ASSERT_OK(db()->Put(wo, "foo", "bar"));
  ASSERT_OK(db()->Put(wo, "foo3", "bar3"));
  ASSERT_OK(db()->Flush(fo));

  std::unique_ptr<Iterator> iter(db()->NewIterator(ro));

  iter->Seek("foo");
  ASSERT_OK(iter->status());
  ASSERT_TRUE(iter->Valid());
  ASSERT_EQ(iter->value().ToString(), "bar");
  EXPECT_EQ(PopTicker(last_options_, NON_LAST_LEVEL_SEEK_FILTER_MATCH), 1U);
  EXPECT_EQ(PopTicker(last_options_, NON_LAST_LEVEL_SEEK_FILTERED), 0U);

  iter->Seek("foo2");
  ASSERT_OK(iter->status());
  ASSERT_TRUE(!iter->Valid());
  EXPECT_EQ(PopTicker(last_options_, NON_LAST_LEVEL_SEEK_FILTER_MATCH), 0U);
  EXPECT_EQ(PopTicker(last_options_, NON_LAST_LEVEL_SEEK_FILTERED), 1U);

  iter->Seek("barbarbar");
  ASSERT_OK(iter->status());
  ASSERT_TRUE(iter->Valid());
  ASSERT_EQ(iter->value().ToString(), "foo");
  EXPECT_EQ(PopTicker(last_options_, NON_LAST_LEVEL_SEEK_FILTER_MATCH), 1U);
  EXPECT_EQ(PopTicker(last_options_, NON_LAST_LEVEL_SEEK_FILTERED), 0U);

  iter->Seek("barfoofoo");
  ASSERT_OK(iter->status());
  ASSERT_TRUE(!iter->Valid());
  EXPECT_EQ(PopTicker(last_options_, NON_LAST_LEVEL_SEEK_FILTER_MATCH), 0U);
  EXPECT_EQ(PopTicker(last_options_, NON_LAST_LEVEL_SEEK_FILTERED), 1U);

  iter->Seek("foobarbar");
  ASSERT_OK(iter->status());
  ASSERT_TRUE(!iter->Valid());
  EXPECT_EQ(PopTicker(last_options_, NON_LAST_LEVEL_SEEK_FILTER_MATCH), 0U);
  EXPECT_EQ(PopTicker(last_options_, NON_LAST_LEVEL_SEEK_FILTERED), 1U);
}

}  // namespace ROCKSDB_NAMESPACE

int main(int argc, char** argv) {
  ROCKSDB_NAMESPACE::port::InstallStackTraceHandler();
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
