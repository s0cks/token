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

    operator leveldb::Slice(){
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
    UserKey(const User& user):
        KeyType(),
        data_(){
      SetTag(ObjectTag(Type::kUser, user.size()));
      SetUser(user);
    }
    UserKey(const leveldb::Slice& slice):
        KeyType(),
        data_(){
      memcpy(data(), slice.data(), std::min(slice.size(), (size_t)kTotalSize));
    }
    ~UserKey() = default;

    ObjectTag tag() const{
      return ObjectTag(*tag_ptr());
    }

    User GetUser() const{
      return User(user_ptr(), kBytesForUser);
    }

    size_t size() const{
      return kTotalSize;
    }

    char* data() const{
      return (char*)data_;
    }

    std::string ToString() const{
      std::stringstream ss;
      ss << "UserKey(";
      ss << "tag=" << tag() << ", ";
      ss << "user=" << GetUser();
      ss << ")";
      return ss.str();
    }
  };

  class PoolKey : public KeyType{
   public:
    static inline int
    Compare(const PoolKey& a, const PoolKey& b){
      int result;
      // compare the objects type first
      if((result = ObjectTag::CompareType(a.tag(), b.tag())) != 0)
        return result; // not equal

      // compare the objects size
      if((result = ObjectTag::CompareSize(a.tag(), b.tag())) != 0)
        return result; // not-equal

      // if the objects are the same type & size, compare the hash.
      return Hash::Compare(a.GetHash(), b.GetHash());
    }
   private:
    enum Layout{
      kTagPosition = 0,
      kBytesForTag = sizeof(RawObjectTag),

      kHashPosition = kTagPosition+kBytesForTag,
      kBytesForHash = Hash::kSize,

      kTotalSize = kHashPosition+kBytesForHash,
    };

    uint8_t data_[kTotalSize];

    inline RawObjectTag*
    tag_ptr() const{
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
    hash_ptr() const{
      return (uint8_t*)&data_[kHashPosition];
    }

    inline void
    SetHash(const Hash& hash){
      memcpy(&data_[kHashPosition], hash.data(), kBytesForHash);
    }
   public:
    PoolKey(const ObjectTag& tag, const Hash& hash):
        KeyType(),
        data_(){
      SetTag(tag);
      SetHash(hash);
    }
    PoolKey(const Type& type, const int16_t& size, const Hash& hash):
      PoolKey(ObjectTag(type, size), hash){}
    PoolKey(const leveldb::Slice& slice):
        KeyType(),
        data_(){
      memcpy(data_, slice.data(), std::min(slice.size(), (size_t)kTotalSize));
    }
    ~PoolKey() = default;

    ObjectTag tag() const{
      return ObjectTag(*tag_ptr());
    }

    Hash GetHash() const{
      return Hash(hash_ptr(), kBytesForHash);
    }

    size_t size() const{
      return kTotalSize;
    }

    char* data() const{
      return (char*)data_;
    }

    std::string ToString() const{
      std::stringstream ss;
      ss << "PoolKey(";
      ss << "tag=" << tag() << ", ";
      ss << "hash=" << GetHash();
      ss << ")";
      return ss.str();
    }
  };
}

#endif//TOKEN_KEY_H