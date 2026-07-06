#ifndef STORAGE_LEVELDB_DB_DB_ITER_H_
#define STORAGE_LEVELDB_DB_DB_ITER_H_

#include <stdint.h>

#include "db/dbformat.h"
#include "leveldb/db.h"

namespace leveldb {

class DBImpl;

Iterator* NewDBIterator(DBImpl* db, const Comparator* user_key_comparator,
                        Iterator* internal_iter, SequenceNumber sequence,
                        uint32_t seed);

}

#endif
