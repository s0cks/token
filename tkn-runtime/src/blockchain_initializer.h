#ifndef TOKEN_BLOCKCHAIN_INITIALIZER_H
#define TOKEN_BLOCKCHAIN_INITIALIZER_H

#include <utility>

#include "blockchain.h"
#include "configuration.h"

namespace token{
#define INITIALIZER_LOG(LevelName) \
  LOG(LevelName) << "[" << GetName() << "Initializer] "

  class BlockChainInitializer{
   protected:
    BlockChainPtr chain_;

    explicit BlockChainInitializer(BlockChainPtr chain):
      chain_(std::move(chain)){}

    inline BlockChainPtr
    GetChain() const{
      return chain_;
    }

    inline Hash
    GetGenesisHash() const{
      return config::GetHash(BLOCKCHAIN_REFERENCE_GENESIS);
    }

    inline Hash
    GetHeadHash() const{
      return config::GetHash(BLOCKCHAIN_REFERENCE_HEAD);
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

    static inline bool
    SetGenesisReference(const Hash& hash) {
      return config::PutProperty(BLOCKCHAIN_REFERENCE_GENESIS, hash);
    }

    static inline bool
    SetHeadReference(const Hash& hash) {
      return config::PutProperty(BLOCKCHAIN_REFERENCE_HEAD, hash);
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
  const char* GetName() const override{ return #Name; }

  class FreshBlockChainInitializer : public BlockChainInitializer{
   public:
    explicit FreshBlockChainInitializer(BlockChainPtr chain):
      BlockChainInitializer(std::move(chain)){}
    ~FreshBlockChainInitializer() override = default;

    DEFINE_INITIALIZER(FreshBlockChain);

    bool InitializeBlockChain() const override{
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
      /*TODO:
        if(!ProcessBlockJob::SubmitAndWait(blk)){
        INITIALIZER_LOG(ERROR) << "couldn't submit new ProcessBlockJob.";
        return TransitionToState(BlockChain::kUninitialized);
      }*/

      SetGenesisReference(hash);
      SetNewHead(hash, blk);
      INITIALIZER_LOG(INFO) << "block chain initialized.";
      return TransitionToState(BlockChain::kInitialized);
    }
  };

  class DefaultBlockChainInitializer : public BlockChainInitializer{
   public:
    explicit DefaultBlockChainInitializer(BlockChainPtr chain):
      BlockChainInitializer(std::move(chain)){}
    ~DefaultBlockChainInitializer() override = default;

    DEFINE_INITIALIZER(DefaultBlockChain);

    bool InitializeBlockChain() const override{
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