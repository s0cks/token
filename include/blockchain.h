#ifndef TOKEN_BLOCKCHAIN_H
#define TOKEN_BLOCKCHAIN_H

#include <map>
#include <set>
#include <leveldb/db.h>
#include <leveldb/comparator.h>

#include "key.h"
#include "block.h"
#include "transaction.h"
#include "merkle/tree.h"
#include "atomic/relaxed_atomic.h"
#include "unclaimed_transaction.h"

namespace token{
#define LOG_CHAIN(LevelName) \
  LOG(LevelName) << "[chain] "
#define DLOG_CHAIN(LevelName) \
  DLOG(LevelName) << "[chain] "

#define BLOCKCHAIN_REFERENCE_GENESIS "<GENESIS>"
#define BLOCKCHAIN_REFERENCE_HEAD "<HEAD>"

#define FOR_EACH_BLOCKCHAIN_STATE(V) \
    V(Uninitialized)                 \
    V(Initializing)                  \
    V(Initialized)                   \
    V(Synchronizing)

  class BlockChain;
  typedef std::shared_ptr<BlockChain> BlockChainPtr;

  static inline std::string
  GetBlockChainDirectory(){
    std::stringstream ss;
    ss << TOKEN_BLOCKCHAIN_HOME << "/data";
    return ss.str();
  }

  class BlockChainBlockVisitor;
  class BlockChain{
    friend class BlockMiner;
    friend class SynchronizeJob; //TODO: revoke access
    friend class BlockChainInitializer;
   public:
    enum State{
#define DEFINE_STATE(Name) k##Name,
      FOR_EACH_BLOCKCHAIN_STATE(DEFINE_STATE)
#undef DEFINE_STATE
    };

    friend std::ostream& operator<<(std::ostream& stream, const State& state){
      switch(state){
#define DEFINE_TOSTRING(Name) \
        case State::k##Name:  \
          return stream << #Name;
        FOR_EACH_BLOCKCHAIN_STATE(DEFINE_TOSTRING)
#undef DEFINE_TOSTRING
        default:
          return stream << "Unknown";
      }
    }

    class BlockKey : public KeyType{
     public:
      static inline int
      CompareSize(const BlockKey& a, const BlockKey& b){
        return ObjectTag::CompareSize(a.tag(), b.tag());
      }

      static inline int
      CompareHash(const BlockKey& a, const BlockKey& b){
        Hash h1 = a.GetHash();
        Hash h2 = b.GetHash();
        if(h1 < h2){
          return -1;
        } else if(h1 > h2){
          return +1;
        }
        return 0;
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
      BlockKey(const int64_t size, const Hash& hash):
        KeyType(),
        data_(){
        SetTag(ObjectTag(Type::kBlock, size));
        SetHash(hash);
      }
      BlockKey(const BlockPtr& blk):
        KeyType(),
        data_(){
        SetTag(blk->GetTag());
        SetHash(blk->GetHash());
      }
      BlockKey(const leveldb::Slice& slice):
        KeyType(),
        data_(){
        memcpy(data(), slice.data(), std::min(slice.size(), (std::size_t)kTotalSize));
      }
      ~BlockKey() = default;

      Hash GetHash() const{
        return Hash(hash_ptr(), kBytesForHash);
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
        ss << "hash=" << GetHash();
        ss << ")";
        return ss.str();
      }

      friend bool operator==(const BlockKey& a, const BlockKey& b){
        return CompareHash(a, b) == 0;
      }

      friend bool operator!=(const BlockKey& a, const BlockKey& b){
        return CompareHash(a, b) != 0;
      }

      friend bool operator<(const BlockKey& a, const BlockKey& b){
        return CompareHash(a, b) < 0;
      }

      friend bool operator>(const BlockKey& a, const BlockKey& b){
        return CompareHash(a, b) > 0;
      }
    };

    class ReferenceKey : public KeyType{
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
          KeyType(),
          data_(){
        SetTag(ObjectTag(Type::kReference, ref.size()));
        SetReference(ref);
      }
      ReferenceKey(const std::string& name):
          ReferenceKey(Reference(name)){}
      ReferenceKey(const leveldb::Slice& slice):
          KeyType(),
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

    class Comparator : public leveldb::Comparator{
     private:
      static inline ObjectTag
      GetTag(const leveldb::Slice& slice){
        ObjectTag tag(*((RawObjectTag*)slice.data()));
        if(!tag.IsValid())
          LOG(WARNING) << "tag " << tag << " is invalid.";
        return tag;
      }
     public:
      Comparator() = default;
      ~Comparator() = default;

      int Compare(const leveldb::Slice& a, const leveldb::Slice& b) const{
        ObjectTag t1 = GetTag(a);
        ObjectTag t2 = GetTag(b);

        int result;
        if((result = ObjectTag::CompareType(t1, t2)) != 0)
          return result; // not equal

        assert(t1.GetType() == t2.GetType());
        if(t1.IsBlockType()){
          BlockKey k1(a);
          if(!k1.valid())
            LOG(WARNING) << "k1 doesn't have a IsValid tag.";

          BlockKey k2(b);
          if(!k2.valid())
            LOG(WARNING) << "k2 doesn't have a IsValid tag.";
          return BlockKey::CompareHash(k1, k2);
        } else if(t1.IsReferenceType()){
          ReferenceKey k1(a);
          if(!k1.valid())
            LOG(WARNING) << "k1 doesn't have a IsValid tag.";

          ReferenceKey k2(b);
          if(!k2.valid())
            LOG(WARNING) << "k2 doesn't have a IsValid tag.";
          return ReferenceKey::CompareCaseInsensitive(k1, k2);
        }

        LOG(ERROR) << "cannot compare keys w/ tag: " << t1 << " <=> " << t2;
        return 0;
      }

      const char* Name() const{
        return "BlockComparator";
      }

      void FindShortestSeparator(std::string* str, const leveldb::Slice& slice) const{}
      void FindShortSuccessor(std::string* str) const {}
    };
   protected:
    RelaxedAtomic<State> state_;
    leveldb::DB* index_;

    inline leveldb::DB*
    GetIndex() const{
      return index_;
    }

    /**
     * TODO
     * @param state
     */
    void SetState(const State& state){
      state_ = state;
    }

    leveldb::Status InitializeIndex(const std::string& filename);
    bool PutBlock(const Hash& hash, BlockPtr blk) const;
    bool RemoveReference(const std::string& name) const;
    bool RemoveBlock(const Hash& hash, const BlockPtr& blk) const;
    bool Append(const BlockPtr& blk);
   public:
    BlockChain():
      state_(State::kUninitialized),
      index_(nullptr){}
    ~BlockChain(){
      delete index_;
    }

    /**
     * TODO
     * @return
     */
    State GetState() const{
      return state_;
    }

    /**
     * TODO
     * @param hash
     * @return
     */
    virtual bool HasBlock(const Hash& hash) const;

    /**
     * TODO
     * @param name
     * @return
     */
    bool HasReference(const std::string& name) const;

    /**
     * TODO
     * @param name
     * @param hash
     * @return
     */
    bool PutReference(const std::string& name, const Hash& hash) const;

    /**
     * TODO
     */
    bool VisitBlocks(BlockChainBlockVisitor* vis) const;

    /**
     * TODO
     * @param name
     * @return
     */
    Hash GetReference(const std::string& name) const;

    /**
     * TODO
     * @param hash
     * @return
     */
    virtual BlockPtr GetBlock(const Hash& hash) const;

    /**
     * TODO
     * @param height
     * @return
     */
    BlockPtr GetBlock(const int64_t& height) const;

    /**
     * TODO
     * @return
     */
    int64_t GetNumberOfBlocks() const;

    /**
     * TODO
     * @return
     */
    int64_t GetNumberOfReferences() const;

    //TODO: guard
    bool GetBlocks(Json::Writer& writer) const;
    bool GetReferences(Json::Writer& writer) const;

    /**
     * TODO
     * @return
     */
    inline bool
    HasBlocks() const{
      return GetNumberOfBlocks() > 0;
    }

    inline bool
    HasReferences() const{
      return GetNumberOfReferences() > 0;
    }

    inline bool
    HasGenesis() const{
      return HasReference(BLOCKCHAIN_REFERENCE_GENESIS);
    }

    inline bool
    HasHead() const{
      return HasReference(BLOCKCHAIN_REFERENCE_HEAD);
    }

    inline Hash
    GetGenesisHash() const{
      return GetReference(BLOCKCHAIN_REFERENCE_GENESIS);
    }

    inline Hash
    GetHeadHash() const{
      return GetReference(BLOCKCHAIN_REFERENCE_HEAD);
    }

    virtual BlockPtr GetHead() const{
      return GetBlock(GetHeadHash());
    }

    inline BlockPtr
    GetGenesis() const{
      return GetBlock(GetGenesisHash());
    }

#define DEFINE_STATE_CHECK(Name) \
    inline bool Is##Name() const{ return GetState() == State::k##Name; }
    FOR_EACH_BLOCKCHAIN_STATE(DEFINE_STATE_CHECK)
#undef DEFINE_STATE_CHECK

    static BlockChainPtr GetInstance();
    static bool Initialize(const std::string& filename=GetBlockChainDirectory());
  };

  class BlockChainBlockVisitor{
   protected:
    BlockChainBlockVisitor() = default;
   public:
    virtual ~BlockChainBlockVisitor() = default;
    virtual bool Visit(const BlockPtr& blk) = 0;
  };
}

#endif //TOKEN_BLOCKCHAIN_H