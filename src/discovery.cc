#include "server.h"
#include "discovery.h"
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

  /*TODO: refactor
  class TransactionPoolBlockBuilder : public ObjectPoolTransactionVisitor{
  //TODO: rebuild class
  private:
      int64_t size_;
      TransactionList transactions_;
  public:
      TransactionPoolBlockBuilder(int64_t size=Block::kMaxTransactionsForBlock):
          ObjectPoolTransactionVisitor(),
          size_(size),
          transactions_(){}
      ~TransactionPoolBlockBuilder(){}

      BlockPtr GetBlock() const{
          // Get the Parent Block
          BlockHeader parent = BlockChain::GetHead()->GetHeader();
          // Sort the Transactions by Timestamp
          //TODO: remove copy
          TransactionList transactions(transactions_);
          std::sort(transactions.begin(), transactions.end(), Transaction::TimestampComparator());
          // Return a New Block of size Block::kMaxTransactionsForBlock, using sorted Transactions
          return std::make_shared<Block>(parent, transactions);
      }

      int64_t GetBlockSize() const{
          return size_;
      }

      int64_t GetNumberOfTransactions() const{
          return transactions_.size();
      }

      bool Visit(const TransactionPtr& tx) const{
          if((GetNumberOfTransactions() + 1) >= GetBlockSize())
              return false;
          transactions_.push_back(tx);
          return true;
      }

      static BlockPtr Build(intptr_t size){
          TransactionPoolBlockBuilder builder(size);
          TransactionPool::Accept(&builder);
          BlockPtr block = builder.GetBlock();
          ObjectPool::PutObject(block->GetHash(), block);
          return block;
      }
  };*/

  static inline void
  OrphanTransaction(const TransactionPtr& tx){
    Hash hash = tx->GetHash();
    LOG(WARNING) << "orphaning transaction: " << hash;
    if(!ObjectPool::RemoveObject(hash))
      LOG(WARNING) << "couldn't remove transaction " << hash << " from pool.";
  }

  static inline void
  OrphanBlock(const BlockPtr& blk){
    Hash hash = blk->GetHash();
    LOG(WARNING) << "orphaning block: " << hash;
    if(!ObjectPool::RemoveObject(hash))
      LOG(WARNING) << "couldn't remove block " << hash << " from pool.";
  }

  BlockPtr BlockDiscoveryThread::CreateNewBlock(intptr_t size){
    BlockPtr blk = Block::Genesis(); //TODO: TransactionPoolBlockBuilder::Build(size);
    SetBlock(blk);
    return blk;
  }

  ProposalPtr BlockDiscoveryThread::CreateNewProposal(BlockPtr blk){
    ProposalPtr proposal = std::make_shared<Proposal>(blk, Server::GetID());
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
    while(!IsStopped()){
      if(HasProposal()){
        ProposalPtr proposal = GetProposal();
        LOG(INFO) << "proposal #" << proposal->GetHeight() << " has been started by " << proposal->GetProposer();
        PeerProposalHandler handler(proposal);
        if(!handler.ProcessProposal()){
          // should we reject the proposal just in-case?
          LOG(WARNING) << "couldn't process proposal #" << proposal->GetHeight() << ".";
          SetProposal(nullptr);
          goto exit;
        }
      } else if(ObjectPool::GetNumberOfTransactions() >= 2){
        BlockPtr blk = CreateNewBlock(2);
        Hash hash = blk->GetHash();
//TODO:
//                if(!BlockVerifier::IsValid(blk, true)){
//                    LOG(WARNING) << "block " << blk << " is invalid, orphaning....";
//                    OrphanBlock(blk);
//                    continue;
//                }

        LOG(INFO) << "discovered block " << hash << ", creating proposal....";
        ProposalPtr proposal = CreateNewProposal(blk);
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
    return Thread::Start(&thread_, "Discovery", &HandleThread, 0);
  }

  bool BlockDiscoveryThread::Stop(){
    if(!IsRunning()){
      LOG(WARNING) << "the block discovery thread is not running.";
      return false;
    }
    return Thread::Stop(thread_);
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