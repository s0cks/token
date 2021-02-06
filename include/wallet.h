#ifndef TOKEN_WALLET_H
#define TOKEN_WALLET_H

#include <ostream>
#include <unordered_set>
#include <leveldb/status.h>
#include <leveldb/comparator.h>
#include <leveldb/write_batch.h>

#include "key.h"
#include "hash.h"
#include "object.h"
#include "utils/buffer.h"

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
    LOG(INFO) << "reading " << len << " tokens from wallet.";
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

#define FOR_EACH_WALLET_MANAGER_STATE(V) \
  V(Uninitialized)                       \
  V(Initializing)                        \
  V(Initialized)

#define FOR_EACH_WALLET_MANAGER_STATUS(V) \
  V(Ok)                                   \
  V(Warning)                              \
  V(Error)

  class WalletManager{
   public:
    enum State{
#define DEFINE_STATE(Name) k##Name,
      FOR_EACH_WALLET_MANAGER_STATE(DEFINE_STATE)
#undef DEFINE_STATE
    };

    friend std::ostream& operator<<(std::ostream& stream, const State& state){
      switch(state){
#define DEFINE_TOSTRING(Name) \
        case State::k##Name:  \
          return stream << #Name;
        FOR_EACH_WALLET_MANAGER_STATE(DEFINE_TOSTRING)
#undef DEFINE_TOSTRING
        default:return stream << "Unknown";
      }
    }

    enum Status{
#define DEFINE_STATUS(Name) k##Name,
      FOR_EACH_WALLET_MANAGER_STATUS(DEFINE_STATUS)
#undef DEFINE_STATUS
    };

    friend std::ostream& operator<<(std::ostream& stream, const Status& status){
      switch(status){
#define DEFINE_TOSTRING(Name) \
        case Status::k##Name: \
          return stream << #Name;
        FOR_EACH_WALLET_MANAGER_STATUS(DEFINE_TOSTRING)
#undef DEFINE_TOSTRING
        default:
          return stream << "Unknown";
      }
    }
   private:
    class Comparator : public leveldb::Comparator{
     public:
      int Compare(const leveldb::Slice& a, const leveldb::Slice& b) const{
        UserKey k1(a);
        if(!k1.valid())
          LOG(WARNING) << "k1 doesn't have a valid tag.";

        UserKey k2(b);
        if(!k2.valid())
          LOG(WARNING) << "k2 doesn't have a valid tag.";
        return UserKey::Compare(k1, k2);
      }

      const char* Name() const{
        return "WalletComparator";
      }

      void FindShortestSeparator(std::string* str, const leveldb::Slice& slice) const{}
      void FindShortSuccessor(std::string* str) const {}
    };
   private:
    WalletManager() = delete;

    static void SetState(const State& state);
    static void SetStatus(const Status& status);
   public:
    ~WalletManager() = delete;

    static State GetState();
    static Status GetStatus();
    static bool Initialize();

    static bool HasWallet(const User& user);
    static bool RemoveWallet(const User& user);
    static bool PutWallet(const User& user, const Wallet& wallet);
    static bool GetWallet(const User& user, Wallet& wallet);
    static bool GetWallet(const User& user, Json::String& json);
    static bool GetWallet(const User& user, Json::Writer& writer);
    static leveldb::Status Write(leveldb::WriteBatch* batch);
    static int64_t GetNumberOfWallets();

    static inline bool
    GetWallet(const std::string& user, Wallet& wallet){
      return GetWallet(User(user), wallet);
    }

    static inline bool
    PutWallet(const std::string& user, const Wallet& wallet){
      return PutWallet(User(user), wallet);
    }

    static inline bool
    RemoveWallet(const std::string& user){
      return RemoveWallet(User(user));
    }

    static inline bool
    HasWallet(const std::string& user){
      return HasWallet(User(user));
    }

#define DEFINE_CHECK(Name) \
    static bool Is##Name(){ return GetState() == State::k##Name; }
    FOR_EACH_WALLET_MANAGER_STATE(DEFINE_CHECK)
#undef DEFINE_CHECK

#define DEFINE_CHECK(Name) \
    static bool Is##Name(){ return GetStatus() == Status::k##Name; }
    FOR_EACH_WALLET_MANAGER_STATUS(DEFINE_CHECK)
#undef DEFINE_CHECK
  };
}

#endif //TOKEN_WALLET_H
