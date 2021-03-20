#ifndef TOKEN_KEY_H
#define TOKEN_KEY_H

#include "object.h"
#include "block.h"

namespace token{
  class KeyType{
   protected:
    KeyType() = default;
   public:
    virtual ~KeyType() = default;
    virtual ObjectTag tag() const = 0;
    virtual size_t size() const = 0;
    virtual char* data() const = 0;
    virtual std::string ToString() const = 0;

    bool valid() const{
      return tag().IsValid();
    }

    explicit operator leveldb::Slice() const{
      return leveldb::Slice(data(), size());
    }

    friend std::ostream& operator<<(std::ostream& stream, const KeyType& key){
      return stream << key.ToString();
    }
  };

  class UserKey : public KeyType{
   public:
    static inline int
    Compare(const UserKey& a, const UserKey& b){
      return strncmp(a.data(), b.data(), kRawUserSize);
    }

    static inline int
    CompareCaseInsensitive(const UserKey& a, const UserKey& b){
      return strncasecmp(a.data(), b.data(), kRawUserSize);
    }
   private:
    enum Layout{
      kTagPosition = 0,
      kBytesForTag = sizeof(RawObjectTag),

      kUserPosition = kTagPosition+kBytesForTag,
      kBytesForUser = kRawUserSize,

      kTotalSize = kUserPosition+kBytesForUser,
    };

    uint8_t data_[kTotalSize];

    RawObjectTag* tag_ptr() const{
      return (RawObjectTag*)&data_[kTagPosition];
    }

    inline void
    SetTag(const RawObjectTag& tag){
      memcpy(&data_[kTagPosition], &tag, kBytesForTag);
    }

    inline void
    SetTag(const ObjectTag& tag){
      return SetTag(tag.raw());
    }

    inline uint8_t*
    user_ptr() const{
      return (uint8_t*)&data_[kUserPosition];
    }

    inline void
    SetUser(const User& user){
      memcpy(&data_[kUserPosition], user.data(), kBytesForUser);
    }
   public:
    explicit UserKey(const User& user):
      KeyType(),
      data_(){
      SetTag(ObjectTag(Type::kUser, user.size()));
      SetUser(user);
    }
    explicit UserKey(const leveldb::Slice& slice):
      KeyType(),
      data_(){
      memcpy(data_, slice.data(), std::min(slice.size(), (size_t)kTotalSize));
    }
    ~UserKey() override = default;

    ObjectTag tag() const override{
      return ObjectTag(*tag_ptr());
    }

    User GetUser() const{
      return User(user_ptr(), kBytesForUser);
    }

    size_t size() const override{
      return kTotalSize;
    }

    char* data() const override{
      return (char*)data_;
    }

    std::string ToString() const override{
      std::stringstream ss;
      ss << "UserKey(";
      ss << "tag=" << tag() << ", ";
      ss << "user=" << GetUser();
      ss << ")";
      return ss.str();
    }
  };
}

#endif//TOKEN_KEY_H