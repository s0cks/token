#ifndef TKN_BATCH_H
#define TKN_BATCH_H

#include <mutex>
#include <glog/logging.h>
#include <leveldb/write_batch.h>
#include "common.h"
#include "atomic/relaxed_atomic.h"

namespace token{
  namespace internal{
    class WriteBatch{
    protected:
      WriteBatch* parent_;
      leveldb::WriteBatch batch_;
    public:
      explicit WriteBatch(WriteBatch* parent):
        parent_(parent),
        batch_(){}
      WriteBatch():
        WriteBatch(nullptr){}
      virtual ~WriteBatch() = default;

      WriteBatch* GetParent() const{
        return parent_;
      }

      leveldb::WriteBatch* batch(){
        return &batch_;
      }

      bool HasParent() const{
        return parent_ != nullptr;
      }

      std::string ToString() const{
        std::stringstream ss;
        ss << "WriteBatch(";
        ss << "size=" << PrettySize(batch_.ApproximateSize());
        ss << ")";
        return ss.str();
      }
    };
  }
}

#endif//TKN_BATCH_H