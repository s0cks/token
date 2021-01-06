#include "server/server.h"
#include "discovery.h"
#include "block_builder.h"
#include "proposal_handler.h"

namespace Token{
  static pthread_t thread_;

  static std::mutex mutex_;
  static std::condition_variable cond_;
  static BlockDiscoveryThread::State state_ = BlockDiscoveryThread::kStopped;
  static BlockDiscoveryThread::Status status_ = BlockDiscoveryThread::kOk;

  static BlockPtr block_ = nullptr;
  static ProposalPtr proposal_;

#define LOCK_GUARD std::lock_guard<std::mutex> guard(mutex_)
#define LOCK std::unique_lock<std::mutex> lock(mutex_)
#define UNLOCK lock.unlock()
#define WAIT cond_.wait(lock)
#define SIGNAL_ONE cond_.notify_one()
#define SIGNAL_ALL cond_.notify_all()

#define CHECK_FOR_REJECTION(Proposal) \
    if((Proposal)->IsRejected()){ \
        OnRejected((Proposal)); \
    }
#define CHECK_STATUS(ProposalInstance) \
    if((ProposalInstance)->GetNumberOfRejected() > (ProposalInstance)->GetNumberOfAccepted()){ \
        (ProposalInstance)->SetStatus(Proposal::Result::kRejected); \
    }

  static inline void
  OrphanTransaction(const TransactionPtr& tx){
    Hash hash = tx->GetHash();
    LOG(WARNING) << "orphaning transaction: " << hash;
    if(!ObjectPool::RemoveTransaction(hash))
      LOG(WARNING) << "couldn't remove transaction " << hash << " from pool.";
  }

  static inline void
  OrphanBlock(const BlockPtr& blk){
    Hash hash = blk->GetHash();
    LOG(WARNING) << "orphaning block: " << hash;
    if(!ObjectPool::RemoveBlock(hash))
      LOG(WARNING) << "couldn't remove block " << hash << " from pool.";
  }

  BlockPtr BlockDiscoveryThread::CreateNewBlock(){
    BlockPtr blk = BlockBuilder::BuildNewBlock();
    SetBlock(blk);
    return blk;
  }

  ProposalPtr BlockDiscoveryThread::CreateNewProposal(BlockPtr blk){
    #ifdef TOKEN_ENABLE_SERVER
    ProposalPtr proposal = std::make_shared<Proposal>(blk, Server::GetID());
    #else
    ProposalPtr proposal = std::make_shared<Proposal>(blk, UUID());
    #endif//TOKEN_ENABLE_SERVER
    SetProposal(proposal);
    return proposal;
  }

  void BlockDiscoveryThread::SetBlock(BlockPtr blk){
    LOCK_GUARD;
    block_ = blk;
  }

  void BlockDiscoveryThread::SetProposal(const ProposalPtr& proposal){
    LOCK_GUARD;
    proposal_ = proposal;
  }

  bool BlockDiscoveryThread::HasProposal(){
    LOCK_GUARD;
    return proposal_ != nullptr;
  }

  BlockPtr BlockDiscoveryThread::GetBlock(){
    LOCK_GUARD;
    return block_;
  }

  ProposalPtr BlockDiscoveryThread::GetProposal(){
    LOCK_GUARD;
    return proposal_;
  }

  void BlockDiscoveryThread::HandleThread(uword parameter){
    LOG(INFO) << "starting the block discovery thread....";
    SetState(BlockDiscoveryThread::kRunning);
    while(IsRunning()){
      if(HasProposal()){
        #ifdef TOKEN_ENABLE_SERVER
        ProposalPtr proposal = GetProposal();
        LOG(INFO) << "proposal #" << proposal->GetHeight() << " has been started by " << proposal->GetProposer();
        PeerProposalHandler handler(proposal);
        if(!handler.ProcessProposal()){
          // should we reject the proposal just in-case?
          LOG(WARNING) << "couldn't process proposal #" << proposal->GetHeight() << ".";
          SetProposal(nullptr);
          goto exit;
        }
        #endif//TOKEN_ENABLE_SERVER
      } else if(ObjectPool::GetNumberOfTransactions() >= Block::kMaxTransactionsForBlock){
        BlockPtr blk = CreateNewBlock();
        Hash hash = blk->GetHash();
//TODO:
//                if(!BlockVerifier::IsValid(blk, true)){
//                    LOG(WARNING) << "block " << blk << " is invalid, orphaning....";
//                    OrphanBlock(blk);
//                    continue;
//                }

        LOG(INFO) << "discovered block " << hash << ", creating proposal....";
        ProposalPtr proposal = CreateNewProposal(blk);
        PeerSessionManager::BroadcastDiscoveredBlock();

        NewProposalHandler handler(proposal);
        if(!handler.ProcessProposal()){
          LOG(WARNING) << "couldn't process proposal #" << proposal->GetHeight() << ".";
          SetProposal(nullptr);
          goto exit;
        }
      }
    }
    exit:
    SetState(BlockDiscoveryThread::kStopped);
    pthread_exit(NULL);
  }

  bool BlockDiscoveryThread::Start(){
    if(!IsStopped()){
      LOG(WARNING) << "the block discovery thread is already running.";
      return false;
    }
    return Thread::StartThread(&thread_, "miner", &HandleThread, 0);
  }

  bool BlockDiscoveryThread::Stop(){
    if(!IsRunning()){
      LOG(WARNING) << "the block discovery thread is not running.";
      return false;
    }
    SetState(BlockDiscoveryThread::kStopping);
    return Thread::StopThread(thread_);
  }

  void BlockDiscoveryThread::SetState(const State& state){
    LOCK;
    state_ = state;
    UNLOCK;
    SIGNAL_ALL;
  }

  void BlockDiscoveryThread::SetStatus(const Status& status){
    LOCK;
    status_ = status;
    UNLOCK;
  }

  void BlockDiscoveryThread::WaitForState(const State& state){
    LOCK;
    while(state_ != state) WAIT;
    UNLOCK;
  }

  BlockDiscoveryThread::State BlockDiscoveryThread::GetState(){
    LOCK_GUARD;
    return state_;
  }

  BlockDiscoveryThread::Status BlockDiscoveryThread::GetStatus(){
    LOCK_GUARD;
    return status_;
  }
}