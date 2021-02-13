#include "blockchain_initializer.h"

namespace token{
#define CANNOT_TRANSITION(From, To) \
  LOG(ERROR) << "cannot transition blockchain from " << (From) << " state to " << (To) << " state.";

  bool BlockChainInitializer::SetNewHead(const Hash& hash, const BlockPtr& blk) const{
    if(!BlockChain::PutBlock(hash, blk)){
#ifdef TOKEN_DEBUG
      LOG(ERROR) << "cannot put new " << BLOCKCHAIN_REFERENCE_HEAD << " block " << blk->ToString();
#else
      LOG(ERROR) << "cannot put new " << BLOCKCHAIN_REFERENCE_HEAD << " block " << hash;
#endif//TOKEN_DEBUG
      return false;
    }
    return SetHeadReference(hash);
  }

  bool BlockChainInitializer::TransitionToState(const BlockChain::State& state) const{
    switch(state){
      case BlockChain::State::kInitializing:{
        if(!BlockChain::IsUninitialized()){
          CANNOT_TRANSITION(BlockChain::GetState(), state);
          return false;
        }
        BlockChain::SetState(state);
        return true;
      }
      case BlockChain::State::kInitialized:{
        if(!BlockChain::IsInitializing() && !BlockChain::IsSynchronizing()){
          CANNOT_TRANSITION(BlockChain::GetState(), state);
          return false;
        }
        BlockChain::SetState(state);
        return true;
      }
      case BlockChain::State::kSynchronizing:{
        if(!BlockChain::IsInitialized()){
          CANNOT_TRANSITION(BlockChain::GetState(), state);
          return false;
        }
        BlockChain::SetState(state);
        return true;
      }
      case BlockChain::State::kUninitialized:{
        if(BlockChain::IsSynchronizing()){
          CANNOT_TRANSITION(BlockChain::GetState(), state);
          return false;
        }
        BlockChain::SetState(state);
        return true;
      }
      default:{
        CANNOT_TRANSITION(BlockChain::GetState(), state);
        return false;
      }
    }
  }
#undef CANNOT_TRANSITION


}