#ifndef TOKEN_WALLET_H
#define TOKEN_WALLET_H

#include <ostream>
#include <unordered_set>

#include <leveldb/db.h>
#include <leveldb/status.h>
#include <leveldb/comparator.h>
#include <leveldb/write_batch.h>

#include "key.h"
#include "hash.h"
#include "object.h"
#include "buffer.h"

#include "atomic/relaxed_atomic.h"

namespace token{
  typedef std::unordered_set<Hash, Hash::Hasher, Hash::Equal> Wallet;

  static inline int64_t
  GetBufferSize(const Wallet& wallet){
    int64_t size = 0;
    size += sizeof(int64_t);
    size += (wallet.size() * Hash::kSize);
    return size;
  }

  static inline bool
  Encode(const Wallet& wallet, BufferPtr& buff){
    if(!buff->PutLong(wallet.size()))
      return false;
    for(auto& it : wallet)
      if(!buff->PutHash(it))
        return false;
    return true;
  }

  static inline bool
  Decode(const std::string& data, Wallet& wallet){
    Buffer buff(data);
    int64_t len = buff.GetLong();
    for(int64_t idx = 0; idx < len; idx++)
      if(!wallet.insert(buff.GetHash()).second)
        return false;
    return true;
  }

  static inline bool
  Decode(BufferPtr& buff, Wallet& wallet){
    int64_t size = buff->GetLong();
    for(int64_t idx = 0; idx < size; idx++)
      if(!wallet.insert(buff->GetHash()).second)
        return false;
    return true;
  }

  void ToJson(const Wallet& hashes, Json::String& json);

  static std::ostream& operator<<(std::ostream& stream, const Wallet& wallet){
    size_t idx = 0;
    stream << "[";
    for(auto& it : wallet){
      stream << it;
      if((++idx < wallet.size()))
        stream << ", ";
    }
    return stream << "]";
  }

#define FOR_EACH_WALLET_MANAGER_STATE(V) \
  V(Uninitialized)                       \
  V(Initializing)                        \
  V(Initialized)

  class WalletManager{
    friend class WalletManagerBatchWriteJob;
   public:
    enum State{
#define DEFINE_STATE(Name) k##Name##State,
      FOR_EACH_WALLET_MANAGER_STATE(DEFINE_STATE)
#undef DEFINE_STATE
    };

    friend std::ostream& operator<<(std::ostream& stream, const State& state){
      switch(state){
#define DEFINE_TOSTRING(Name) \
        case State::k##Name##State:  \
          return stream << #Name;
        FOR_EACH_WALLET_MANAGER_STATE(DEFINE_TOSTRING)
#undef DEFINE_TOSTRING
        default:
          return stream << "Unknown";
      }
    }

    class Comparator : public leveldb::Comparator{
     public:
      int Compare(const leveldb::Slice& a, const leveldb::Slice& b) const{
        UserKey k1(a);
        if(!k1.valid())
          LOG(WARNING) << "k1 doesn't have a IsValid tag.";

        UserKey k2(b);
        if(!k2.valid())
          LOG(WARNING) << "k2 doesn't have a IsValid tag.";
        return UserKey::Compare(k1, k2);
      }

      const char* Name() const{
        return "WalletComparator";
      }

      void FindShortestSeparator(std::string* str, const leveldb::Slice& slice) const{}
      void FindShortSuccessor(std::string* str) const {}
    };
   protected:
    RelaxedAtomic<State> state_;
    leveldb::DB* index_;

    void SetState(const State& state){
      state_ = state;
    }

    inline leveldb::DB*
    GetIndex() const{
      return index_;
    }

    bool LoadIndex(const std::string& filename);
    leveldb::Status Commit(const leveldb::WriteBatch& batch);
   public:
    WalletManager():
      state_(State::kUninitializedState),
      index_(nullptr){}
    virtual ~WalletManager(){
      if(index_)
        delete index_;
    }

    State GetState() const{
      return state_;
    }

    bool HasWallet(const User& user) const;
    bool RemoveWallet(const User& user) const;
    bool PutWallet(const User& user, const Wallet& wallet) const;
    bool GetWallet(const User& user, Wallet& wallet) const;
    bool GetWallet(const User& user, Json::Writer& writer) const;
    int64_t GetNumberOfWallets() const;

    inline bool
    GetWallet(const std::string& user, Wallet& wallet) const{
      return GetWallet(User(user), wallet);
    }

    inline bool
    PutWallet(const std::string& user, const Wallet& wallet) const{
      return PutWallet(User(user), wallet);
    }

    inline bool
    RemoveWallet(const std::string& user) const{
      return RemoveWallet(User(user));
    }

    inline bool
    HasWallet(const std::string& user) const{
      return HasWallet(User(user));
    }

#define DEFINE_CHECK(Name) \
    bool Is##Name() const{ return GetState() == State::k##Name##State; }
    FOR_EACH_WALLET_MANAGER_STATE(DEFINE_CHECK)
#undef DEFINE_CHECK

    static inline std::string
    GetWalletManagerFilename(){
      std::stringstream filename;
      filename << TOKEN_BLOCKCHAIN_HOME << "/wallets";
      return filename.str();
    }

    static bool Initialize(const std::string& filename=GetWalletManagerFilename());
    static WalletManager* GetInstance();
  };
}

#endif //TOKEN_WALLET_H
