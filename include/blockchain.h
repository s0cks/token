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
#include "unclaimed_transaction.h"

namespace token{
#define BLOCKCHAIN_REFERENCE_GENESIS "<GENESIS>"
#define BLOCKCHAIN_REFERENCE_HEAD "<HEAD>"

#define FOR_EACH_BLOCKCHAIN_STATE(V) \
    V(Uninitialized)                 \
    V(Initializing)                  \
    V(Initialized)                   \
    V(Synchronizing)

  class BlockChainBlockVisitor;
  class BlockChainHeaderVisitor;
  class BlockChain{
    friend class BlockChainInitializer;

    friend class BlockMiner;
    friend class ProposalHandler;
    friend class SynchronizeJob; //TODO: revoke access
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
   private:
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
            LOG(WARNING) << "k1 doesn't have a valid tag.";

          BlockKey k2(b);
          if(!k2.valid())
            LOG(WARNING) << "k2 doesn't have a valid tag.";
          return BlockKey::CompareHeight(k1, k2);
        }

        ReferenceKey k1(a);
        if(!k1.valid())
          LOG(WARNING) << "k1 doesn't have a valid tag.";

        ReferenceKey k2(b);
        if(!k2.valid())
          LOG(WARNING) << "k2 doesn't have a valid tag.";
        return ReferenceKey::CompareCaseInsensitive(k1, k2);
      }

      const char* Name() const{
        return "BlockComparator";
      }

      void FindShortestSeparator(std::string* str, const leveldb::Slice& slice) const{}
      void FindShortSuccessor(std::string* str) const {}
    };
   private:
    BlockChain() = delete;

    static inline bool
    ShouldInstallFresh(){ //TODO: rename function/action
#ifdef TOKEN_DEBUG
      return !BlockChain::HasReference(BLOCKCHAIN_REFERENCE_GENESIS) || FLAGS_fresh;
#else
      return !BlockChain::HasReference(BLOCKCHAIN_REFERENCE_GENESIS);
#endif//TOKEN_DEBUG
    }

    static leveldb::DB* GetIndex();
    static void SetState(const State& state);
    static bool PutBlock(const Hash& hash, BlockPtr blk);
    static bool PutReference(const std::string& name, const Hash& hash);
    static bool RemoveReference(const std::string& name);
    static bool RemoveBlock(const Hash& hash, const BlockPtr& blk);
    static bool Append(const BlockPtr& blk);
   public:
    ~BlockChain() = delete;

    static State GetState();
    static bool Initialize();
    static bool VisitHeaders(BlockChainHeaderVisitor* vis);
    static bool VisitBlocks(BlockChainBlockVisitor* vis);
    static bool HasBlock(const Hash& hash);
    static bool HasReference(const std::string& name);
    static Hash GetReference(const std::string& name);
    static BlockPtr GetBlock(const Hash& hash);
    static BlockPtr GetBlock(int64_t height);
    static BlockPtr GetHead();
    static BlockPtr GetGenesis();
    static int64_t GetNumberOfBlocks();

    static bool GetBlocks(Json::Writer& writer);

    #ifdef TOKEN_DEBUG
    static bool GetStats(Json::Writer& writer);
    #endif//TOKEN_DEBUG

    static inline bool HasBlocks(){
      return GetNumberOfBlocks() > 0;
    }

    static inline bool
    HasHead(){
      return !GetReference(BLOCKCHAIN_REFERENCE_HEAD).IsNull();
    }

    static inline bool
    HasGenesis(){
      return !GetReference(BLOCKCHAIN_REFERENCE_GENESIS).IsNull();
    }

#define DEFINE_STATE_CHECK(Name) \
        static inline bool Is##Name(){ return GetState() == State::k##Name; }
    FOR_EACH_BLOCKCHAIN_STATE(DEFINE_STATE_CHECK)
#undef DEFINE_STATE_CHECK
  };

  class BlockChainBlockVisitor{
   protected:
    BlockChainBlockVisitor() = default;
   public:
    virtual ~BlockChainBlockVisitor() = default;
    virtual bool Visit(const BlockPtr& blk) = 0;
  };

  class BlockChainHeaderVisitor{
   protected:
    BlockChainHeaderVisitor() = delete;
   public:
    virtual ~BlockChainHeaderVisitor() = default;
    virtual bool Visit(const BlockHeader& blk) = 0;
  };
}

#endif //TOKEN_BLOCKCHAIN_H