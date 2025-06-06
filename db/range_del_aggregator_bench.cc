//  Copyright (c) 2018-present, Facebook, Inc.  All rights reserved.
//  This source code is licensed under both the GPLv2 (found in the
//  COPYING file in the root directory) and Apache 2.0 License
//  (found in the LICENSE.Apache file in the root directory).

#ifndef GFLAGS
#include <cstdio>
int main() {
  fprintf(stderr, "Please install gflags to run rocksdb tools\n");
  return 1;
}
#else

#include <iomanip>
#include <iostream>
#include <memory>
#include <random>
#include <set>
#include <string>
#include <vector>

#include "db/dbformat.h"
#include "db/range_del_aggregator.h"
#include "db/range_tombstone_fragmenter.h"
#include "rocksdb/comparator.h"
#include "rocksdb/system_clock.h"
#include "util/coding.h"
#include "util/gflags_compat.h"
#include "util/random.h"
#include "util/stop_watch.h"
#include "util/vector_iterator.h"

using GFLAGS_NAMESPACE::ParseCommandLineFlags;

DEFINE_int32(num_range_tombstones, 1000, "number of range tombstones created");

DEFINE_int32(num_runs, 1000, "number of test runs");

DEFINE_int32(tombstone_start_upper_bound, 1000,
             "exclusive upper bound on range tombstone start keys");

DEFINE_int32(should_delete_upper_bound, 1000,
             "exclusive upper bound on keys passed to ShouldDelete");

DEFINE_double(tombstone_width_mean, 100.0, "average range tombstone width");

DEFINE_double(tombstone_width_stddev, 0.0,
              "standard deviation of range tombstone width");

DEFINE_int32(seed, 0, "random number generator seed");

DEFINE_int32(should_deletes_per_run, 1, "number of ShouldDelete calls per run");

DEFINE_int32(add_tombstones_per_run, 1,
             "number of AddTombstones calls per run");

DEFINE_bool(use_compaction_range_del_aggregator, false,
            "Whether to use CompactionRangeDelAggregator. Default is to use "
            "ReadRangeDelAggregator.");

namespace {

struct Stats {
  uint64_t time_add_tombstones = 0;
  uint64_t time_first_should_delete = 0;
  uint64_t time_rest_should_delete = 0;
  uint64_t time_fragment_tombstones = 0;
};

std::ostream& operator<<(std::ostream& os, const Stats& s) {
  std::ios fmt_holder(nullptr);
  fmt_holder.copyfmt(os);

  os << std::left;
  os << std::setw(25) << "Fragment Tombstones: "
     << s.time_fragment_tombstones /
            (FLAGS_add_tombstones_per_run * FLAGS_num_runs * 1.0e3)
     << " us\n";
  os << std::setw(25) << "AddTombstones: "
     << s.time_add_tombstones /
            (FLAGS_add_tombstones_per_run * FLAGS_num_runs * 1.0e3)
     << " us\n";
  os << std::setw(25) << "ShouldDelete (first): "
     << s.time_first_should_delete / (FLAGS_num_runs * 1.0e3) << " us\n";
  if (FLAGS_should_deletes_per_run > 1) {
    os << std::setw(25) << "ShouldDelete (rest): "
       << s.time_rest_should_delete /
              ((FLAGS_should_deletes_per_run - 1) * FLAGS_num_runs * 1.0e3)
       << " us\n";
  }

  os.copyfmt(fmt_holder);
  return os;
}

auto icmp = ROCKSDB_NAMESPACE::InternalKeyComparator(
    ROCKSDB_NAMESPACE::BytewiseComparator());

}  // anonymous namespace

namespace ROCKSDB_NAMESPACE {

namespace {

// A wrapper around RangeTombstones and the underlying data of its start and end
// keys.
struct PersistentRangeTombstone {
  std::string start_key;
  std::string end_key;
  RangeTombstone tombstone;

  PersistentRangeTombstone(std::string start, std::string end,
                           SequenceNumber seq)
      : start_key(std::move(start)), end_key(std::move(end)) {
    tombstone = RangeTombstone(start_key, end_key, seq);
  }

  PersistentRangeTombstone() = default;

  PersistentRangeTombstone(const PersistentRangeTombstone& t) { *this = t; }

  PersistentRangeTombstone& operator=(const PersistentRangeTombstone& t) {
    start_key = t.start_key;
    end_key = t.end_key;
    tombstone = RangeTombstone(start_key, end_key, t.tombstone.seq_);

    return *this;
  }

  PersistentRangeTombstone(PersistentRangeTombstone&& t) noexcept { *this = t; }

  PersistentRangeTombstone& operator=(PersistentRangeTombstone&& t) {
    start_key = std::move(t.start_key);
    end_key = std::move(t.end_key);
    tombstone = RangeTombstone(start_key, end_key, t.tombstone.seq_);

    return *this;
  }
};

struct TombstoneStartKeyComparator {
  explicit TombstoneStartKeyComparator(const Comparator* c) : cmp(c) {}

  bool operator()(const RangeTombstone& a, const RangeTombstone& b) const {
    return cmp->Compare(a.start_key_, b.start_key_) < 0;
  }

  const Comparator* cmp;
};

std::unique_ptr<InternalIterator> MakeRangeDelIterator(
    const std::vector<PersistentRangeTombstone>& range_dels) {
  std::vector<std::string> keys, values;
  for (const auto& range_del : range_dels) {
    auto key_and_value = range_del.tombstone.Serialize();
    keys.push_back(key_and_value.first.Encode().ToString());
    values.push_back(key_and_value.second.ToString());
  }
  return std::unique_ptr<VectorIterator>(
      new VectorIterator(keys, values, &icmp));
}

// convert long to a big-endian slice key
static std::string Key(int64_t val) {
  std::string little_endian_key;
  std::string big_endian_key;
  PutFixed64(&little_endian_key, val);
  assert(little_endian_key.size() == sizeof(val));
  big_endian_key.resize(sizeof(val));
  for (size_t i = 0; i < sizeof(val); ++i) {
    big_endian_key[i] = little_endian_key[sizeof(val) - 1 - i];
  }
  return big_endian_key;
}

}  // anonymous namespace

}  // namespace ROCKSDB_NAMESPACE

int main(int argc, char** argv) {
  ParseCommandLineFlags(&argc, &argv, true);

  Stats stats;
  ROCKSDB_NAMESPACE::SystemClock* clock =
      ROCKSDB_NAMESPACE::SystemClock::Default().get();
  ROCKSDB_NAMESPACE::Random64 rnd(FLAGS_seed);
  std::default_random_engine random_gen(FLAGS_seed);
  std::normal_distribution<double> normal_dist(FLAGS_tombstone_width_mean,
                                               FLAGS_tombstone_width_stddev);
  std::vector<std::vector<ROCKSDB_NAMESPACE::PersistentRangeTombstone> >
      all_persistent_range_tombstones(FLAGS_add_tombstones_per_run);
  for (int i = 0; i < FLAGS_add_tombstones_per_run; i++) {
    all_persistent_range_tombstones[i] =
        std::vector<ROCKSDB_NAMESPACE::PersistentRangeTombstone>(
            FLAGS_num_range_tombstones);
  }
  auto mode = ROCKSDB_NAMESPACE::RangeDelPositioningMode::kForwardTraversal;
  std::vector<ROCKSDB_NAMESPACE::SequenceNumber> snapshots{0};
  for (int i = 0; i < FLAGS_num_runs; i++) {
    std::unique_ptr<ROCKSDB_NAMESPACE::RangeDelAggregator> range_del_agg =
        nullptr;
    if (FLAGS_use_compaction_range_del_aggregator) {
      range_del_agg.reset(new ROCKSDB_NAMESPACE::CompactionRangeDelAggregator(
          &icmp, snapshots));
    } else {
      range_del_agg.reset(new ROCKSDB_NAMESPACE::ReadRangeDelAggregator(
          &icmp, ROCKSDB_NAMESPACE::kMaxSequenceNumber /* upper_bound */));
    }

    std::vector<
        std::unique_ptr<ROCKSDB_NAMESPACE::FragmentedRangeTombstoneList> >
        fragmented_range_tombstone_lists(FLAGS_add_tombstones_per_run);

    for (auto& persistent_range_tombstones : all_persistent_range_tombstones) {
      // TODO(abhimadan): consider whether creating the range tombstones right
      // before AddTombstones is artificially warming the cache compared to
      // real workloads.
      for (int j = 0; j < FLAGS_num_range_tombstones; j++) {
        uint64_t start = rnd.Uniform(FLAGS_tombstone_start_upper_bound);
        uint64_t end = static_cast<uint64_t>(
            std::round(start + std::max(1.0, normal_dist(random_gen))));
        persistent_range_tombstones[j] =
            ROCKSDB_NAMESPACE::PersistentRangeTombstone(
                ROCKSDB_NAMESPACE::Key(start), ROCKSDB_NAMESPACE::Key(end), j);
      }
      auto iter =
          ROCKSDB_NAMESPACE::MakeRangeDelIterator(persistent_range_tombstones);
      ROCKSDB_NAMESPACE::StopWatchNano stop_watch_fragment_tombstones(
          clock, true /* auto_start */);
      fragmented_range_tombstone_lists.emplace_back(
          new ROCKSDB_NAMESPACE::FragmentedRangeTombstoneList(
              std::move(iter), icmp, FLAGS_use_compaction_range_del_aggregator,
              snapshots));
      stats.time_fragment_tombstones +=
          stop_watch_fragment_tombstones.ElapsedNanos();
      std::unique_ptr<ROCKSDB_NAMESPACE::FragmentedRangeTombstoneIterator>
          fragmented_range_del_iter(
              new ROCKSDB_NAMESPACE::FragmentedRangeTombstoneIterator(
                  fragmented_range_tombstone_lists.back().get(), icmp,
                  ROCKSDB_NAMESPACE::kMaxSequenceNumber));

      ROCKSDB_NAMESPACE::StopWatchNano stop_watch_add_tombstones(
          clock, true /* auto_start */);
      range_del_agg->AddTombstones(std::move(fragmented_range_del_iter));
      stats.time_add_tombstones += stop_watch_add_tombstones.ElapsedNanos();
    }

    ROCKSDB_NAMESPACE::ParsedInternalKey parsed_key;
    parsed_key.sequence = FLAGS_num_range_tombstones / 2;
    parsed_key.type = ROCKSDB_NAMESPACE::kTypeValue;

    uint64_t first_key = rnd.Uniform(FLAGS_should_delete_upper_bound -
                                     FLAGS_should_deletes_per_run + 1);

    for (int j = 0; j < FLAGS_should_deletes_per_run; j++) {
      std::string key_string = ROCKSDB_NAMESPACE::Key(first_key + j);
      parsed_key.user_key = key_string;

      ROCKSDB_NAMESPACE::StopWatchNano stop_watch_should_delete(
          clock, true /* auto_start */);
      range_del_agg->ShouldDelete(parsed_key, mode);
      uint64_t call_time = stop_watch_should_delete.ElapsedNanos();

      if (j == 0) {
        stats.time_first_should_delete += call_time;
      } else {
        stats.time_rest_should_delete += call_time;
      }
    }
  }

  std::cout << "=========================\n"
            << "Results:\n"
            << "=========================\n"
            << stats;

  return 0;
}

#endif  // GFLAGS
