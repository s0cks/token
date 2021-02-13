#ifndef TOKEN_BLOCKCHAIN_INITIALIZER_H
#define TOKEN_BLOCKCHAIN_INITIALIZER_H

#include "blockchain.h"

#include "job/processor.h"
#include "job/scheduler.h"

namespace token{
#define INITIALIZER_LOG(LevelName) \
  LOG(LevelName) << "[" << GetName() << "Initializer] "

  class BlockChainInitializer{
   protected:
    BlockChainInitializer() = default;

    inline bool
    SetGenesisReference(const Hash& hash) const{
      return BlockChain::PutReference(BLOCKCHAIN_REFERENCE_GENESIS, hash);
    }

    inline bool
    SetHeadReference(const Hash& hash) const{
      return BlockChain::PutReference(BLOCKCHAIN_REFERENCE_HEAD, hash);
    }

    inline bool
    PruneBlock(const Hash& hash, const BlockPtr& blk) const{
      INITIALIZER_LOG(INFO) << "pruning " << hash << "....";
      return BlockChain::RemoveBlock(hash, blk);
    }

    bool SetNewHead(const Hash& hash, const BlockPtr& blk) const;
    bool TransitionToState(const BlockChain::State& state) const;
   public:
    virtual ~BlockChainInitializer() = default;
    virtual const char* GetName() const = 0;
    virtual bool InitializeBlockChain() const = 0;
  };

#define DEFINE_INITIALIZER(Name) \
  const char* GetName() const{ return #Name; }

  class FreshBlockChainInitializer : public BlockChainInitializer{
   public:
    FreshBlockChainInitializer():
      BlockChainInitializer(){}
    ~FreshBlockChainInitializer() = default;

    DEFINE_INITIALIZER(FreshBlockChain);

    bool InitializeBlockChain() const{
      INITIALIZER_LOG(INFO) << "initializing block chain....";
      if(!TransitionToState(BlockChain::kInitializing))
        return TransitionToState(BlockChain::kUninitialized);

      if(BlockChain::HasBlocks()){
        INITIALIZER_LOG(WARNING) << "pruning local block chain.....";
        BlockPtr current = BlockChain::GetHead();
        while(current->GetHeight() > 0){
          Hash hash = current->GetHash();
          if(!PruneBlock(hash, current))
            return false;
          current = BlockChain::GetBlock(current->GetPreviousHash());
        }

#ifdef TOKEN_DEBUG
        INITIALIZER_LOG(INFO) << "pruning done.";
#endif//TOKEN_DEBUG
      }

      BlockPtr blk = Block::Genesis();
      Hash hash = blk->GetHash();
#ifdef TOKEN_DEBUG
      INITIALIZER_LOG(INFO) << "using genesis block: " << blk->ToString();
#else
      INITIALIZER_LOG(INFO) << "using genesis block: " << hash;
#endif//TOKEN_DEBUG

      // [Before - Work Stealing]
      //  - ProcessGenesisBlock Timeline (19s)
      // [After - Work Stealing]
      //  - ProcessGenesisBlock Timeline (4s)
      INITIALIZER_LOG(INFO) << "processing genesis....";
      JobQueue* queue = JobScheduler::GetThreadQueue();
      ProcessBlockJob* job = new ProcessBlockJob(blk);
      queue->Push(job);
      while(!job->IsFinished()); //spin

      SetGenesisReference(hash);
      SetNewHead(hash, blk);
      if(!TransitionToState(BlockChain::kInitialized))
        return TransitionToState(BlockChain::kUninitialized);

      INITIALIZER_LOG(INFO) << "block chain initialized.";
      return true;
    }
  };

  class DefaultBlockChainInitializer : public BlockChainInitializer{
   public:
    DefaultBlockChainInitializer() = default;
    ~DefaultBlockChainInitializer() = default;

    DEFINE_INITIALIZER(DefaultBlockChain);

    bool InitializeBlockChain() const{
      INITIALIZER_LOG(INFO) << "initializing block chain....";
      if(!TransitionToState(BlockChain::kInitializing))
        return TransitionToState(BlockChain::kUninitialized);

      if(!TransitionToState(BlockChain::kInitialized))
        return TransitionToState(BlockChain::kUninitialized);
      INITIALIZER_LOG(INFO) << "block chain initialized.";
      return true;
    }
  };
}

#endif//TOKEN_BLOCKCHAIN_INITIALIZER_H