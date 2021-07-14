#include "blockchain_initializer.h"

namespace token{
#define CANNOT_TRANSITION(From, To) \
  LOG(ERROR) << "cannot transition blockchain from " << (From) << " state to " << (To) << " state.";

  bool BlockChainInitializer::SetNewHead(const Hash& hash, const BlockPtr& blk) const{
    if(!GetChain()->PutBlock(hash, blk)){
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
        if(!GetChain()->IsUninitialized()){
          CANNOT_TRANSITION(GetChain()->GetState(), state);
          return false;
        }
        GetChain()->SetState(state);
        return true;
      }
      case BlockChain::State::kInitialized:{
        if(!GetChain()->IsInitializing() && !GetChain()->IsSynchronizing()){
          CANNOT_TRANSITION(GetChain()->GetState(), state);
          return false;
        }
        GetChain()->SetState(state);
        return true;
      }
      case BlockChain::State::kSynchronizing:{
        if(!GetChain()->IsInitialized()){
          CANNOT_TRANSITION(GetChain()->GetState(), state);
          return false;
        }
        GetChain()->SetState(state);
        return true;
      }
      case BlockChain::State::kUninitialized:{
        if(GetChain()->IsSynchronizing()){
          CANNOT_TRANSITION(GetChain()->GetState(), state);
          return false;
        }
        GetChain()->SetState(state);
        return true;
      }
      default:{
        CANNOT_TRANSITION(GetChain()->GetState(), state);
        return false;
      }
    }
  }
#undef CANNOT_TRANSITION


}