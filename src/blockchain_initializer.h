#ifndef TOKEN_BLOCKCHAIN_INITIALIZER_H
#define TOKEN_BLOCKCHAIN_INITIALIZER_H

#include "reference.h"
#include "blockchain.h"
#include "job/processor.h"
#include "job/scheduler.h"

namespace token{
#define INITIALIZER_LOG(LevelName) \
  LOG(LevelName) << "[" << GetName() << "Initializer] "

  class BlockChainInitializer{
   protected:
    BlockChainPtr chain_;
    ReferenceDatabasePtr references_;

    BlockChainInitializer(const BlockChainPtr& chain, const ReferenceDatabasePtr& references):
      chain_(chain),
      references_(references){}

    inline BlockChainPtr
    GetChain() const{
      return chain_;
    }

    inline ReferenceDatabasePtr
    GetReferences() const{
      return references_;
    }

    inline Hash
    GetGenesisHash() const{
      return GetReferences()->GetReference(BLOCKCHAIN_REFERENCE_GENESIS);
    }

    inline Hash
    GetHeadHash() const{
      return GetReferences()->GetReference(BLOCKCHAIN_REFERENCE_HEAD);
    }

    inline BlockPtr
    GetGenesis() const{
      return GetChain()->GetGenesis();
    }

    inline BlockPtr
    GetHead() const{
      return GetChain()->GetHead();
    }

    inline BlockPtr
    GetBlock(const Hash& hash) const{
      return GetChain()->GetBlock(hash);
    }

    inline bool
    SetGenesisReference(const Hash& hash) const{
      return GetReferences()->PutReference(BLOCKCHAIN_REFERENCE_GENESIS, hash);
    }

    inline bool
    SetHeadReference(const Hash& hash) const{
      return GetReferences()->PutReference(BLOCKCHAIN_REFERENCE_HEAD, hash);
    }

    inline bool
    PruneBlock(const Hash& hash, const BlockPtr& blk) const{
      INITIALIZER_LOG(INFO) << "pruning " << hash << "....";
      return GetChain()->RemoveBlock(hash, blk);
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
    FreshBlockChainInitializer(const BlockChainPtr& chain, const ReferenceDatabasePtr& references):
      BlockChainInitializer(chain, references){}
    ~FreshBlockChainInitializer() = default;

    DEFINE_INITIALIZER(FreshBlockChain);

    bool InitializeBlockChain() const{
      INITIALIZER_LOG(INFO) << "initializing block chain....";
      if(!TransitionToState(BlockChain::kInitializing))
        return TransitionToState(BlockChain::kUninitialized);

      if(GetChain()->HasBlocks()){
        //TODO: refactor
        INITIALIZER_LOG(WARNING) << "pruning local block chain.....";
        BlockPtr current = GetHead();
        while(current->height() > 0){
          Hash hash = current->hash();
          if(!PruneBlock(hash, current))
            return false;
          current = GetBlock(current->GetPreviousHash());
        }

#ifdef TOKEN_DEBUG
        INITIALIZER_LOG(INFO) << "pruning done.";
#endif//TOKEN_DEBUG
      }

      BlockPtr blk = Block::Genesis();
      Hash hash = blk->hash();
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
      if(!ProcessBlockJob::SubmitAndWait(blk)){
        INITIALIZER_LOG(ERROR) << "couldn't submit new ProcessBlockJob.";
        return TransitionToState(BlockChain::kUninitialized);
      }

      SetGenesisReference(hash);
      SetNewHead(hash, blk);
      INITIALIZER_LOG(INFO) << "block chain initialized.";
      return TransitionToState(BlockChain::kInitialized);
    }
  };

  class DefaultBlockChainInitializer : public BlockChainInitializer{
   public:
    DefaultBlockChainInitializer(const BlockChainPtr& chain, const ReferenceDatabasePtr& references):
      BlockChainInitializer(chain, references){}
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