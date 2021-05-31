
#pragma once

#pragma warning(push)
#pragma warning(disable: 4100 4267)
#if ETH_ROCKSDB
#include <rocksdb/db.h>
#include <rocksdb/write_batch.h>
namespace ldb = rocksdb;
#else
#include <leveldb/db.h>
#include <leveldb/write_batch.h>
namespace ldb = leveldb;
#endif
#pragma warning(pop)
#define DEV_LDB 1
