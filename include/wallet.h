#ifndef TOKEN_WALLET_H
#define TOKEN_WALLET_H

#include <unordered_set>

#include "hash.h"
#include "user.h"
#include "json.h"
#include "object.h"

namespace token{
  typedef std::unordered_set<Hash, Hash::Hasher, Hash::Equal> Wallet;

  int64_t GetBufferSize(const Wallet&);
  bool Encode(const BufferPtr&, const Wallet&);
  bool Decode(const std::string&, Wallet&);
  bool Decode(const BufferPtr&, Wallet&);

  static inline std::ostream&
  operator<<(std::ostream& stream, const Wallet& wallet){
    size_t idx = 0;
    stream << "[";
    for(auto& it : wallet){
      stream << it;
      if((++idx < wallet.size()))
        stream << ", ";
    }
    return stream << "]";
  }

  namespace json{
    static inline bool
    SetField(Writer& writer, const char* name, const Wallet& val){
      KEY(ERROR, writer, name);
      START_ARRAY(ERROR, writer);
      for (auto& it : val){
        std::string hex = it.HexString();
        if (!writer.String(hex.data(), hex.length())){
          LOG(ERROR) << "cannot append " << it << " to json array.";
          return false;
        }
      }
      END_ARRAY(ERROR, writer);
      return true;
    }
  }

  class UserWallet{
   protected:
    User user_;
    Wallet wallet_;

   public:
    UserWallet():
      user_(),
      wallet_(){}
    UserWallet(const User& user, const Wallet& wallet):
      user_(user),
      wallet_(wallet){}
    UserWallet(const UserWallet& other) = default;
    ~UserWallet() = default;

    User& user(){
      return user_;
    }

    User user() const{
      return user_;
    }

    Wallet& wallet(){
      return wallet_;
    }

    Wallet wallet() const{
      return wallet_;
    }

    UserWallet& operator=(const UserWallet& other) = default;

    friend bool operator==(const UserWallet& a, const UserWallet& b){
      return a.user_ == b.user_
          && a.wallet_ == b.wallet_;
    }

    friend bool operator!=(const UserWallet& a, const UserWallet& b){
      return !operator==(a, b);
    }

    friend std::ostream& operator<<(std::ostream& stream, const UserWallet& val){
      stream << "UserWallet(";
      stream << "user=" << val.user() << ", ";
      stream << "wallet=" << val.wallet();
      stream << ")";
      return stream;
    }

    friend json::Writer& operator<<(json::Writer& writer, const UserWallet& val){
      if(!writer.StartObject()){
        DLOG(ERROR) << "cannot start UserWallet json object.";
        return writer;
      }

      if(!json::SetField(writer, "user", val.user())){
        DLOG(ERROR) << "cannot set user field for UserWallet json object.";
        return writer;
      }

      if(!json::SetField(writer, "wallet", val.wallet())){
        DLOG(ERROR) << "cannot set wallet field for UserWallet json object.";
        return writer;
      }
      return writer;
    }
  };

  namespace json{
    static inline bool
    SetField(Writer& writer, const char* name, const UserWallet& val){
      if(!writer.Key(name)){
        DLOG(ERROR) << "cannot start field name '" << name << "' for json object.";//TODO: cleanup
        return false;
      }

      writer << val;
      return true;
    }
  }
}

#endif //TOKEN_WALLET_H
