// Copyright (c) Facebook, Inc. and its affiliates. All Rights Reserved.
// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#pragma once

#include <stdint.h>

#include <memory>
#include <string>

#include "rocksdb/table.h"

namespace ROCKSDB_NAMESPACE {

struct EnvOptions;

class Status;
class RandomAccessFile;
class WritableFile;
class Table;
class TableBuilder;

// PlainTableFactory is the entrance function to the PlainTable format of
// SST files. It returns instances PlainTableBuilder as the builder
// class and PlainTableReader as the reader class, where the format is
// actually implemented.
//
// The PlainTable is designed for memory-mapped file systems, e.g. tmpfs.
// Data is not organized in blocks, which allows fast access. Because of
// following downsides
// 1. Data compression is not supported.
// 2. Data is not checksumed.
// it is not recommended to use this format on other type of file systems.
//
// PlainTable requires fixed length key, configured as a constructor
// parameter of the factory class. Output file format:
// +-------------+-----------------+
// | version     | user_key_length |
// +------------++------------+-----------------+  <= key1 offset
// |  encoded key1            | value_size  |   |
// +------------+-------------+-------------+   |
// | value1                                     |
// |                                            |
// +--------------------------+-------------+---+  <= key2 offset
// | encoded key2             | value_size  |   |
// +------------+-------------+-------------+   |
// | value2                                     |
// |                                            |
// |        ......                              |
// +-----------------+--------------------------+
//
// When the key encoding type is kPlain. Key part is encoded as:
// +------------+--------------------+
// | [key_size] |  internal key      |
// +------------+--------------------+
// for the case of user_key_len = kPlainTableVariableLength case,
// and simply:
// +----------------------+
// |  internal key        |
// +----------------------+
// for user_key_len != kPlainTableVariableLength case.
//
// If key encoding type is kPrefix. Keys are encoding in this format.
// There are three ways to encode a key:
// (1) Full Key
// +---------------+---------------+-------------------+
// | Full Key Flag | Full Key Size | Full Internal Key |
// +---------------+---------------+-------------------+
// which simply encodes a full key
//
// (2) A key shared the same prefix as the previous key, which is encoded as
//     format of (1).
// +-------------+-------------+-------------+-------------+------------+
// | Prefix Flag | Prefix Size | Suffix Flag | Suffix Size | Key Suffix |
// +-------------+-------------+-------------+-------------+------------+
// where key is the suffix part of the key, including the internal bytes.
// the actual key will be constructed by concatenating prefix part of the
// previous key, with the suffix part of the key here, with sizes given here.
//
// (3) A key shared the same prefix as the previous key, which is encoded as
//     the format of (2).
// +-----------------+-----------------+------------------------+
// | Key Suffix Flag | Key Suffix Size | Suffix of Internal Key |
// +-----------------+-----------------+------------------------+
// The key will be constructed by concatenating previous key's prefix (which is
// also a prefix which the last key encoded in the format of (1)) and the
// key given here.
//
// For example, we for following keys (prefix and suffix are separated by
// spaces):
//   0000 0001
//   0000 00021
//   0000 0002
//   00011 00
//   0002 0001
// Will be encoded like this:
//   FK 8 00000001
//   PF 4 SF 5 00021
//   SF 4 0002
//   FK 7 0001100
//   FK 8 00020001
// (where FK means full key flag, PF means prefix flag and SF means suffix flag)
//
// All those "key flag + key size" shown above are in this format:
// The 8 bits of the first byte:
// +----+----+----+----+----+----+----+----+
// |  Type   |            Size             |
// +----+----+----+----+----+----+----+----+
// Type indicates: full key, prefix, or suffix.
// The last 6 bits are for size. If the size bits are not all 1, it means the
// size of the key. Otherwise, varint32 is read after this byte. This varint
// value + 0x3F (the value of all 1) will be the key size.
//
// For example, full key with length 16 will be encoded as (binary):
//     00 010000
// (00 means full key)
// and a prefix with 100 bytes will be encoded as:
//     01 111111    00100101
//         (63)       (37)
// (01 means key suffix)
//
// All the internal keys above (including kPlain and kPrefix) are encoded in
// this format:
// There are two types:
// (1) normal internal key format
// +----------- ...... -------------+----+---+---+---+---+---+---+---+
// |       user key                 |type|      sequence ID          |
// +----------- ..... --------------+----+---+---+---+---+---+---+---+
// (2) Special case for keys whose sequence ID is 0 and is value type
// +----------- ...... -------------+----+
// |       user key                 |0x80|
// +----------- ..... --------------+----+
// To save 7 bytes for the special case where sequence ID = 0.
//
//
class PlainTableFactory : public TableFactory {
 public:
  ~PlainTableFactory() {}
  // user_key_len is the length of the user key. If it is set to be
  // kPlainTableVariableLength, then it means variable length. Otherwise, all
  // the keys need to have the fix length of this value. bloom_bits_per_key is
  // number of bits used for bloom filer per key. hash_table_ratio is
  // the desired utilization of the hash table used for prefix hashing.
  // hash_table_ratio = number of prefixes / #buckets in the hash table
  // hash_table_ratio = 0 means skip hash table but only replying on binary
  // search.
  // index_sparseness determines index interval for keys
  // inside the same prefix. It will be the maximum number of linear search
  // required after hash and binary search.
  // index_sparseness = 0 means index for every key.
  // huge_page_tlb_size determines whether to allocate hash indexes from huge
  // page TLB and the page size if allocating from there. See comments of
  // Arena::AllocateAligned() for details.
  explicit PlainTableFactory(
      const PlainTableOptions& _table_options = PlainTableOptions());

  // Method to allow CheckedCast to work for this class
  static const char* kClassName() { return kPlainTableName(); }
  const char* Name() const override { return kPlainTableName(); }
  using TableFactory::NewTableReader;
  Status NewTableReader(const ReadOptions& ro,
                        const TableReaderOptions& table_reader_options,
                        std::unique_ptr<RandomAccessFileReader>&& file,
                        uint64_t file_size, std::unique_ptr<TableReader>* table,
                        bool prefetch_index_and_filter_in_cache) const override;

  TableBuilder* NewTableBuilder(
      const TableBuilderOptions& table_builder_options,
      WritableFileWriter* file) const override;

  std::string GetPrintableOptions() const override;
  static const char kValueTypeSeqId0 = char(~0);

  std::unique_ptr<TableFactory> Clone() const override {
    return std::make_unique<PlainTableFactory>(*this);
  }

 private:
  PlainTableOptions table_options_;
};

}  // namespace ROCKSDB_NAMESPACE
