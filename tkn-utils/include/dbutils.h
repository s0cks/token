#ifndef TKN_DBUTILS_H
#define TKN_DBUTILS_H

#include <leveldb/db.h>
#include <leveldb/slice.h>
#include <leveldb/status.h>
#include <leveldb/comparator.h>

namespace token{
  static inline std::ostream&
  operator<<(std::ostream& stream, const leveldb::Status& status){
    return stream << status.ToString();
  }
}

#endif//TKN_DBUTILS_H