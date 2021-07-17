#ifndef TOKEN_WALLET_MANAGER_H
#define TOKEN_WALLET_MANAGER_H

#include "type/user.h"
#include "type/wallet.h"
#include "flags.h"
#include "kvstore.h"
#include "atomic/relaxed_atomic.h"

namespace token{
#define LOG_WALLETS(LevelName) \
  LOG(LevelName) << "[wallets] "

#define DLOG_WALLETS(LevelName) \
  DLOG(LevelName) << "[wallets] "

#define DLOG_WALLETS_IF(LevelName, Condition) \
  DLOG_IF(LevelName, Condition) << "[wallets] "

  class WalletManager : public internal::KeyValueStore{
    friend class WalletManagerBatchWriteJob;
   public:
    class Comparator : public leveldb::Comparator{
     public:
      int Compare(const leveldb::Slice& a, const leveldb::Slice& b) const override{
        User k1(a), k2(b);
        return User::Compare(k1, k2);
      }

      const char* Name() const override{
        return "WalletComparator";
      }

      void FindShortestSeparator(std::string* str, const leveldb::Slice& slice) const override{}
      void FindShortSuccessor(std::string* str) const override {}
    };
   protected:
    leveldb::Status Commit(const leveldb::WriteBatch& batch);
   public:
    WalletManager() = default;
    virtual ~WalletManager() = default;

    virtual bool HasWallet(const User& user) const;
    virtual bool RemoveWallet(const User& user) const;
    virtual bool PutWallet(const User& user, const Wallet& wallet) const;
    virtual Wallet GetUserWallet(const User& user) const;

    virtual bool GetWallet(const User& user, Wallet& wallet) const;
    virtual int64_t GetNumberOfWallets() const;

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

    static inline std::string
    GetWalletManagerFilename(){
      std::stringstream filename;
      filename << TOKEN_BLOCKCHAIN_HOME << "/wallets";
      return filename.str();
    }

    static inline const char*
    GetName(){
      return "WalletManager";
    }

    static inline WalletManagerPtr
    NewInstance(){
      return std::make_shared<WalletManager>();
    }

    static WalletManagerPtr GetInstance();
    static bool Initialize(const std::string& filename=GetWalletManagerFilename());
  };
}

#endif//TOKEN_WALLET_MANAGER_H