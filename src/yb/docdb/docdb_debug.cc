// Copyright (c) YugaByte, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except
// in compliance with the License.  You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software distributed under the License
// is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express
// or implied.  See the License for the specific language governing permissions and limitations
// under the License.
//

#include "yb/docdb/docdb_debug.h"

#include "yb/docdb/docdb_fwd.h"
#include "yb/docdb/shared_lock_manager_fwd.h"
#include "yb/docdb/docdb-internal.h"
#include "yb/docdb/key_bounds.h"
#include "yb/docdb/kv_debug.h"

#include "yb/rocksdb/db.h"

#include "yb/util/bytes_formatter.h"
#include "yb/util/result.h"

using namespace std::placeholders;

namespace yb {
namespace docdb {

namespace {

void AppendLineToStream(const std::string& s, ostream* out) {
  *out << s << std::endl;
}

template <class DumpStringFunc>
void ProcessDumpEntry(
    Slice key, Slice value, const SchemaPackingStorage& schema_packing_storage,
    StorageDbType db_type, IncludeBinary include_binary, DumpStringFunc func) {
  const auto key_str = DocDBKeyToDebugStr(key, db_type);
  if (!key_str.ok()) {
    func(key_str.status().ToString());
    return;
  }
  const KeyType key_type = GetKeyType(key, db_type);
  Result<std::string> value_str = DocDBValueToDebugStr(
      key_type, *key_str, value, schema_packing_storage);
  if (!value_str.ok()) {
    func(value_str.status().CloneAndAppend(". Key: " + *key_str).ToString());
  } else {
    func(Format("$0 -> $1", *key_str, *value_str));
  }
  if (include_binary) {
    func(Format("$0 -> $1\n", FormatSliceAsStr(key), FormatSliceAsStr(value)));
  }
}

template <class DumpStringFunc>
void DocDBDebugDump(
    rocksdb::DB* rocksdb, const SchemaPackingStorage& schema_packing_storage, StorageDbType db_type,
    IncludeBinary include_binary, DumpStringFunc dump_func) {
  rocksdb::ReadOptions read_opts;
  read_opts.query_id = rocksdb::kDefaultQueryId;
  auto iter = std::unique_ptr<rocksdb::Iterator>(rocksdb->NewIterator(read_opts));
  iter->SeekToFirst();

  while (iter->Valid()) {
    ProcessDumpEntry(
        iter->key(), iter->value(), schema_packing_storage, db_type, include_binary, dump_func);
    iter->Next();
  }
}

} // namespace

std::string EntryToString(
    const Slice& key, const Slice& value, const SchemaPackingStorage& schema_packing_storage,
    StorageDbType db_type) {
  std::ostringstream out;
  ProcessDumpEntry(
      key, value, schema_packing_storage, db_type, IncludeBinary::kFalse,
      std::bind(&AppendLineToStream, _1, &out));
  return out.str();
}

std::string EntryToString(
    const rocksdb::Iterator& iterator, const SchemaPackingStorage& schema_packing_storage,
    StorageDbType db_type) {
  return EntryToString(iterator.key(), iterator.value(), schema_packing_storage, db_type);
}

void DocDBDebugDump(
    rocksdb::DB* rocksdb, ostream& out, const SchemaPackingStorage& schema_packing_storage,
    StorageDbType db_type, IncludeBinary include_binary) {
  DocDBDebugDump(
      rocksdb, schema_packing_storage, db_type, include_binary,
      std::bind(&AppendLineToStream, _1, &out));
}

std::string DocDBDebugDumpToStr(
    DocDB docdb, const SchemaPackingStorage& schema_packing_storage, IncludeBinary include_binary) {
  std::stringstream ss;
  DocDBDebugDump(
      docdb.regular, ss, schema_packing_storage, StorageDbType::kRegular, include_binary);
  if (docdb.intents) {
    DocDBDebugDump(
        docdb.intents, ss, schema_packing_storage, StorageDbType::kIntents, include_binary);
  }
  return ss.str();
}

std::string DocDBDebugDumpToStr(
    rocksdb::DB* rocksdb, const SchemaPackingStorage& schema_packing_storage, StorageDbType db_type,
    IncludeBinary include_binary) {
  std::stringstream ss;
  DocDBDebugDump(rocksdb, ss, schema_packing_storage, db_type, include_binary);
  return ss.str();
}

void AppendToContainer(const std::string& s, std::unordered_set<std::string>* out) {
  out->insert(s);
}

void AppendToContainer(const std::string& s, std::vector<std::string>* out) {
  out->push_back(s);
}

template <class T>
void DocDBDebugDumpToContainer(
    rocksdb::DB* rocksdb, const SchemaPackingStorage& schema_packing_storage, T* out,
    StorageDbType db_type) {
  void (*f)(const std::string&, T*) = AppendToContainer;
  DocDBDebugDump(
      rocksdb, schema_packing_storage, db_type, IncludeBinary::kFalse, std::bind(f, _1, out));
}

void DocDBDebugDumpToContainer(
    DocDB docdb, const SchemaPackingStorage& schema_packing_storage,
    std::unordered_set<std::string>* out) {
  DocDBDebugDumpToContainer(docdb.regular, schema_packing_storage, out, StorageDbType::kRegular);
  if (docdb.intents) {
    DocDBDebugDumpToContainer(docdb.intents, schema_packing_storage, out, StorageDbType::kIntents);
  }
}

void DumpRocksDBToLog(
    rocksdb::DB* rocksdb, const SchemaPackingStorage& schema_packing_storage, StorageDbType db_type,
    const std::string& log_prefix) {
  std::vector<std::string> lines;
  DocDBDebugDumpToContainer(rocksdb, schema_packing_storage, &lines, db_type);
  LOG(INFO) << log_prefix << AsString(db_type) << " DB dump:";
  for (const auto& line : lines) {
    LOG(INFO) << log_prefix << "  " << line;
  }
}

}  // namespace docdb
}  // namespace yb
