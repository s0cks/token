#ifndef TOKEN_KEY_H
#define TOKEN_KEY_H

#include "object.h"
#include "block.h"

namespace token{
  class Key{
   protected:
    Key() = default;
   public:
    virtual ~Key() = default;
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

    friend std::ostream& operator<<(std::ostream& stream, const Key& key){
      return stream << key.ToString();
    }
  };

  class ReferenceKey : public Key{
   public:
    static inline int
    Compare(const ReferenceKey& a, const ReferenceKey& b){
      return strncmp(a.data(), b.data(), kRawReferenceSize);
    }

    static inline int
    CompareCaseInsensitive(const ReferenceKey& a, const ReferenceKey& b){
      return strncasecmp(a.data(), b.data(), kRawReferenceSize);
    }
   private:
    enum Layout{
      kTagPosition = 0,
      kBytesForTag = sizeof(RawObjectTag),

      kReferencePosition = kTagPosition+kBytesForTag,
      kBytesForReference = kRawReferenceSize,

      kTotalSize = kReferencePosition+kBytesForReference,
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
    ref_ptr() const{
      return (uint8_t*)&data_[kReferencePosition];
    }

    inline void
    SetReference(const Reference& ref){
      memcpy(&data_[kReferencePosition], ref.data(), kBytesForReference);
    }
   public:
    ReferenceKey(const Reference& ref):
      Key(),
      data_(){
      SetTag(ObjectTag(Type::kReference, ref.size()));
      SetReference(ref);
    }
    ReferenceKey(const std::string& name):
      ReferenceKey(Reference(name)){}
    ReferenceKey(const leveldb::Slice& slice):
      Key(),
      data_(){
      memcpy(data(), slice.data(), kTotalSize);
    }
    ~ReferenceKey() = default;

    ObjectTag tag() const{
      return ObjectTag(*tag_ptr());
    }

    Reference GetReference() const{
      return Reference(ref_ptr(), kBytesForReference);
    }

    size_t size() const{
      return kTotalSize;
    }

    char* data() const{
      return (char*)data_;
    }

    std::string ToString() const{
      std::stringstream ss;
      ss << "ReferenceKey(";
      ss << "tag=" << tag() << ", ";
      ss << "ref=" << GetReference();
      ss << ")";
      return ss.str();
    }
  };

  class UserKey : public Key{
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
      Key(),
      data_(){
      SetTag(ObjectTag(Type::kUser, user.size()));
      SetUser(user);
    }
    UserKey(const leveldb::Slice& slice):
      Key(),
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

  class BlockKey : public Key{
   public:
    static inline int
    Compare(const BlockKey& a, const BlockKey& b){
      return memcmp(a.data(), b.data(), kTotalSize);
    }

    static inline int
    CompareHeight(const BlockKey& a, const BlockKey& b){
      if(a.GetHeight() < b.GetHeight()){
        return -1;
      } else if(a.GetHeight() > b.GetHeight()){
        return +1;
      }
      return 0;
    }

    static inline int
    CompareSize(const BlockKey& a, const BlockKey& b){
      return ObjectTag::CompareSize(a.tag(), b.tag());
    }
   private:
    enum Layout{
      kTagPosition = 0,
      kBytesForTag = sizeof(RawObjectTag),

      kHeightPosition = kTagPosition+kBytesForTag,
      kBytesForHeight = sizeof(int64_t),

      kHashPosition = kHeightPosition+kBytesForHeight,
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

    inline int64_t*
    height_ptr() const{
      return (int64_t*)&data_[kHeightPosition];
    }

    inline void
    SetHeight(const int64_t& height){
      memcpy(&data_[kHeightPosition], &height, kBytesForHeight);
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
    BlockKey(const int64_t size, const int64_t height, const Hash& hash):
      Key(),
      data_(){
      SetTag(ObjectTag(Type::kBlock, size));
      SetHeight(height);
      SetHash(hash);
    }
    BlockKey(const BlockPtr& blk):
      Key(),
      data_(){
      SetTag(blk->GetTag());
      SetHeight(blk->GetHeight());
      SetHash(blk->GetHash());
    }
    BlockKey(const leveldb::Slice& slice):
      Key(),
      data_(){
      memcpy(data(), slice.data(), std::min(slice.size(), (std::size_t)kTotalSize));
    }
    ~BlockKey() = default;

    Hash GetHash() const{
      return Hash(hash_ptr(), kBytesForHash);
    }

    int64_t GetHeight() const{
      return *height_ptr();
    }

    ObjectTag tag() const{
      return ObjectTag(*tag_ptr());
    }

    size_t size() const{
      return kTotalSize;
    }

    char* data() const{
      return (char*)data_;
    }

    std::string ToString() const{
      std::stringstream ss;
      ss << "BlockKey(";
      ss << "tag=" << tag() << ", ";
      ss << "height=" << GetHeight() << ", ";
      ss << "hash=" << GetHash();
      ss << ")";
      return ss.str();
    }

    friend bool operator==(const BlockKey& a, const BlockKey& b){
      return Compare(a, b) == 0;
    }

    friend bool operator!=(const BlockKey& a, const BlockKey& b){
      return Compare(a, b) != 0;
    }
  };

  class PoolKey : public Key{
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
      Key(),
      data_(){
      SetTag(tag);
      SetHash(hash);
    }
    PoolKey(const Type& type, const int16_t& size, const Hash& hash):
      PoolKey(ObjectTag(type, size), hash){}
    PoolKey(const leveldb::Slice& slice):
      Key(),
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