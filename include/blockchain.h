#ifndef TOKEN_BLOCKCHAIN_H
#define TOKEN_BLOCKCHAIN_H

#include <map>
#include <set>
#include <leveldb/db.h>
#include <leveldb/comparator.h>

#include "key.h"
#include "flags.h"
#include "block.h"
#include "transaction.h"
#include "merkle/tree.h"
#include "configuration.h"
#include "relaxed_atomic.h"
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

  static inline std::string
  GetBlockChainDirectory(){
    std::stringstream ss;
    ss << TOKEN_BLOCKCHAIN_HOME << "/data";
    return ss.str();
  }

  class BlockChainBlockVisitor;
  class BlockChain : public std::enable_shared_from_this<BlockChain>{
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

    class Comparator : public leveldb::Comparator{
     public:
      Comparator() = default;
      ~Comparator() override = default;

      int Compare(const leveldb::Slice& a, const leveldb::Slice& b) const override{
        ObjectKey k1(a), k2(b);
        return ObjectKey::Compare(k1, k2);
      }

      const char* Name() const override{
        return "BlockComparator";
      }

      void FindShortestSeparator(std::string* str, const leveldb::Slice& slice) const override{}
      void FindShortSuccessor(std::string* str) const override{}
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
    bool PutBlock(const Hash& hash, const BlockPtr& blk) const;
    bool RemoveBlock(const Hash& hash, const BlockPtr& blk) const;
    bool Append(const BlockPtr& blk);
   public:
    explicit BlockChain():
      state_(State::kUninitialized),
      index_(nullptr){}
    virtual ~BlockChain(){
      delete index_;
    }

    /**
     * TODO
     * @return
     */
    BlockChain::State GetState() const{
      return (BlockChain::State)state_;
    }

    /**
     * TODO
     * @param hash
     * @return
     */
    virtual bool HasBlock(const Hash& hash) const;

    /**
     * TODO
     */
    bool VisitBlocks(BlockChainBlockVisitor* vis) const;

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
    int64_t GetNumberOfReferences() const{
      NOT_IMPLEMENTED(WARNING);//TODO: implement
      return 0;
    }

    //TODO: guard
    bool GetBlocks(json::Writer& writer) const;

    /**
     * TODO
     * @return
     */
    inline bool
    HasBlocks() const{
      return GetNumberOfBlocks() > 0;
    }

    virtual inline Hash
    GetHeadHash() const{
      return config::GetHash(BLOCKCHAIN_REFERENCE_HEAD);
    }

    virtual inline BlockPtr
    GetHead() const{
      return GetBlock(GetHeadHash());
    }

    virtual inline Hash
    GetGenesisHash() const{
      return config::GetHash(BLOCKCHAIN_REFERENCE_GENESIS);
    }

    virtual inline BlockPtr
    GetGenesis() const{
      return GetBlock(GetGenesisHash());
    }

#define DEFINE_STATE_CHECK(Name) \
    inline bool Is##Name() const{ return GetState() == State::k##Name; }
    FOR_EACH_BLOCKCHAIN_STATE(DEFINE_STATE_CHECK)
#undef DEFINE_STATE_CHECK

    static inline const char*
    GetName(){
      return "BlockChain";
    }

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