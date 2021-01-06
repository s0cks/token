#ifndef TOKEN_WALLET_H
#define TOKEN_WALLET_H

#include <ostream>
#include <unordered_set>
#include <leveldb/status.h>
#include <leveldb/write_batch.h>

#include "hash.h"
#include "object.h"
#include "utils/json_conversion.h"

namespace Token{
  typedef std::unordered_set<Hash, Hash::Hasher, Hash::Equal> Wallet;

  static inline int64_t
  GetBufferSize(const Wallet& wallet){
    return wallet.size() * Hash::kSize;
  }

  static inline bool
  Encode(const Wallet& wallet, BufferPtr& buff){
    if(!buff->PutLong(wallet.size())){
      return false;
    }
    for(auto& it : wallet)
      if(!buff->PutHash(it)){
        return false;
      }
    return true;
  }

  static inline bool
  Decode(BufferPtr& buff, Wallet& wallet){
    int64_t size = buff->GetLong();
    for(int64_t idx = 0; idx < size; idx++)
      if(!wallet.insert(buff->GetHash()).second){
        return false;
      }
    return true;
  }

  void ToJson(const Wallet& hashes, JsonString& json);

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
        default:return stream << "Unknown";
      }
    }
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
    static bool GetWallet(const User& user, JsonString& json);
    static leveldb::Status Write(leveldb::WriteBatch* batch);
    static int64_t GetNumberOfWallets();

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
