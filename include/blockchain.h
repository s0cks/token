#ifndef TOKEN_BLOCKCHAIN_H
#define TOKEN_BLOCKCHAIN_H

#include <map>
#include <set>
#include <leveldb/db.h>

#include "block.h"
#include "transaction.h"
#include "merkle/tree.h"
#include "utils/printer.h"
#include "unclaimed_transaction.h"

namespace Token{
#define BLOCKCHAIN_REFERENCE_GENESIS "<GENESIS>"
#define BLOCKCHAIN_REFERENCE_HEAD "<HEAD>"

#define FOR_EACH_BLOCKCHAIN_STATE(V) \
    V(Uninitialized)                 \
    V(Initializing)                  \
    V(Initialized)                   \
    V(Synchronizing)                 \

#define FOR_EACH_BLOCKCHAIN_STATUS(V) \
    V(Ok)                             \
    V(Warning)                        \
    V(Error)

  class BlockChainStats{
   private:
    BlockHeader genesis_;
    BlockHeader head_;
   public:
    BlockChainStats(const BlockHeader& genesis, const BlockHeader& head):
      genesis_(genesis),
      head_(head){}
    BlockChainStats(const BlockChainStats& stats):
      genesis_(stats.genesis_),
      head_(stats.head_){}
    ~BlockChainStats() = default;

    BlockHeader& GetGenesis(){
      return genesis_;
    }

    BlockHeader GetGenesis() const{
      return genesis_;
    }

    BlockHeader& GetHead(){
      return head_;
    }

    BlockHeader GetHead() const{
      return head_;
    }

    void operator=(const BlockChainStats& stats){
      head_ = stats.head_;
      genesis_ = stats.genesis_;
    }
  };

  class BlockChainBlockVisitor;
  class BlockChainHeaderVisitor;
  class BlockChain{
    friend class Server;
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

    enum Status{
#define DEFINE_STATUS(Name) k##Name,
      FOR_EACH_BLOCKCHAIN_STATUS(DEFINE_STATUS)
#undef DEFINE_STATUS
    };

    friend std::ostream& operator<<(std::ostream& stream, const Status& status){
      switch(status){
#define DEFINE_TOSTRING(Name) \
        case Status::k##Name: \
            return stream << #Name;
        FOR_EACH_BLOCKCHAIN_STATUS(DEFINE_TOSTRING)
#undef DEFINE_TOSTRING
        default:
          return stream << "Unknown";
      }
    }
   private:
    BlockChain() = delete;
    static leveldb::DB* GetIndex();
    static void SetState(State state);
    static void SetStatus(Status status);
    static bool PutBlock(const Hash& hash, BlockPtr blk);
    static bool PutReference(const std::string& name, const Hash& hash);
    static bool RemoveReference(const std::string& name);
    static bool Append(const BlockPtr& blk);
   public:
    ~BlockChain() = delete;

    static State GetState();
    static Status GetStatus();
    static bool Initialize();
    static bool GetBlocks(HashList& hashes);
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
    static BlockChainStats GetStats();

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

#define DEFINE_STATUS_CHECK(Name) \
        static inline bool Is##Name(){ return GetStatus() == Status::k##Name; }
    FOR_EACH_BLOCKCHAIN_STATUS(DEFINE_STATUS_CHECK)
#undef DEFINE_STATUS_CHECK
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