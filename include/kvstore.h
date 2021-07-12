#ifndef TOKEN_KVSTORE_H
#define TOKEN_KVSTORE_H

#include <memory>
#include <ostream>
#include <leveldb/db.h>
#include <leveldb/slice.h>
#include <leveldb/status.h>
#include <leveldb/comparator.h>
#include <glog/logging.h>

#include "common.h"
#include "atomic/relaxed_atomic.h"

namespace token{
  namespace internal{
#define FOR_EACH_KVSTORE_STATE(V) \
    V(Uninitialized)              \
    V(Initializing)               \
    V(Initialized)

    class KeyValueStore{
     public:
      enum State{
#define DEFINE_STATE(Name) k##Name##State,
        FOR_EACH_KVSTORE_STATE(DEFINE_STATE)
#undef DEFINE_STATE
      };

      friend std::ostream& operator<<(std::ostream& stream, const State& state){
        switch(state){
#define DEFINE_TOSTRING(Name) \
          case State::k##Name##State: \
            return stream << #Name;
          FOR_EACH_KVSTORE_STATE(DEFINE_TOSTRING)
#undef DEFINE_TOSTRING
          default:
            return stream << "Unknown";
        }
      }
     protected:
      RelaxedAtomic<State> state_;
      leveldb::DB* index_;

      KeyValueStore():
        state_(State::kUninitializedState),
        index_(nullptr){}

      inline void
      SetState(const State& state){
        state_ = state;
      }

      inline leveldb::DB*
      GetIndex() const{
        return index_;
      }

      template<class Comparator>
      leveldb::Status InitializeIndex(const std::string& filename){
        if(!IsUninitialized())
          return leveldb::Status::NotSupported("Cannot re-initialize the index.");

        LOG(INFO) << "initializing.....";
        DLOG(INFO) << "index filename: " << filename;
        SetState(State::kInitializingState);
        if(!FileExists(filename)){
          DLOG(WARNING) << "creating index " << filename << "....";
          if(!CreateDirectory(filename)){
            DLOG(ERROR) << "couldn't create the index directory: " << filename;
            SetState(State::kUninitializedState);

            std::stringstream ss;
            ss << "Couldn't create the index directory: " << filename;
            return leveldb::Status::IOError(ss.str());
          }
        }

        leveldb::Options options;
        options.create_if_missing = true;
        options.comparator = new Comparator();

        leveldb::Status status;
        if(!(status = leveldb::DB::Open(options, filename, &index_)).ok()){
          LOG(FATAL) << "Couldn't initialize the " << filename << " index: " << status.ToString();
          SetState(State::kUninitializedState);
          return status;
        }

        SetState(State::kInitializedState);
        DLOG(INFO) << "initialized.";
        return status;
      }
     public:
      virtual ~KeyValueStore() = default;

      State GetState() const{
        return (State)state_;
      }

#define DEFINE_CHECK(Name) \
      inline bool Is##Name(){ return GetState() == State::k##Name##State; }
      FOR_EACH_KVSTORE_STATE(DEFINE_CHECK)
#undef DEFINE_CHECK
    };
  }
}

#endif//TOKEN_KVSTORE_H