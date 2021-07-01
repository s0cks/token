#include <glog/logging.h>
#include <leveldb/status.h>

#include "common.h"
#include "reference.h"

namespace token{
  ReferenceDatabasePtr ReferenceDatabase::GetInstance(){
    static const ReferenceDatabasePtr instance = ReferenceDatabase::NewInstance();
    return instance;
  }

  bool ReferenceDatabase::Initialize(const std::string& filename){
    leveldb::Status status;
    if(!(status = GetInstance()->InitializeIndex<ReferenceDatabase::Comparator>(filename)).ok()){
      LOG(ERROR) << "cannot initialize the ReferenceDatabase: " << status.ToString();
      return false;
    }
    return true;
  }

  bool ReferenceDatabase::HasReference(const Reference& key) const{
    std::string value;
    leveldb::Status status;
    if(!(status = GetIndex()->Get(leveldb::ReadOptions(), (const leveldb::Slice&)key, &value)).ok()){
      DLOG(WARNING) << "cannot find reference " << key << ": " << status.ToString();
      return false;
    }
    return true;
  }

  bool ReferenceDatabase::PutReference(const Reference& key, const Hash& val) const{
    leveldb::WriteOptions options;
    options.sync = true;

    std::string value = val.HexString();

    leveldb::Status status;
    if(!(status = GetIndex()->Put(options, (const leveldb::Slice&)key, value)).ok()){
      LOG(ERROR) << "cannot set reference " << key << " to " << val << ": " << status.ToString();
      return false;
    }

    DLOG(INFO) << "set reference " << key << " to " << val;
    return true;
  }

  bool ReferenceDatabase::RemoveReference(const Reference& key) const{
    std::string value;
    leveldb::Status status;
    if(!(status = GetIndex()->Get(leveldb::ReadOptions(), (const leveldb::Slice&)key, &value)).ok()){
      DLOG(WARNING) << "cannot find reference " << key << ": " << status.ToString();
      return false;
    }
    return true;
  }

  Hash ReferenceDatabase::GetReference(const Reference& key) const{
    std::string value;
    leveldb::Status status;

    if(!(status = GetIndex()->Get(leveldb::ReadOptions(), (const leveldb::Slice&)key, &value)).ok()){
      if(status.IsNotFound()){
        DLOG(WARNING) << "cannot find reference: " << key;
        return Hash();
      }

      LOG(ERROR) << "cannot get reference " << key << ": " << status.ToString();
      return Hash();
    }
    return Hash::FromHexString(value);
  }

  int64_t ReferenceDatabase::GetNumberOfReferences() const{
    return 0;//TODO: implement
  }
}