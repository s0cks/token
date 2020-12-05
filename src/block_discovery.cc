#include "block_discovery.h"
#include "block_chain.h"
#include "async_task.h"
#include "peer.h"
#include "server.h"
#include "block_validator.h"
#include "block_processor.h"
#include "transaction_validator.h"

namespace Token{
    static std::recursive_mutex mutex_;
    static std::condition_variable_any cond_;
    static Thread::State state_ = Thread::State::kStopped;

    static Block* block_ = nullptr;
    static Proposal* proposal_ = nullptr;

#define LOCK_GUARD std::lock_guard<std::recursive_mutex> guard(mutex_)
#define LOCK std::unique_lock<std::recursive_mutex> lock(mutex_)
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

    static inline size_t
    GetNumberOfTransactionsInPool(){
        return TransactionPool::GetSize();
    }

    class TransactionPoolBlockBuilder : public TransactionPoolVisitor{
    private:
        int64_t size_;
        TransactionList transactions_;
    public:
        TransactionPoolBlockBuilder(int64_t size=Block::kMaxTransactionsForBlock):
            TransactionPoolVisitor(),
            size_(size),
            transactions_(){}
        ~TransactionPoolBlockBuilder(){}

        Block* GetBlock() const{
            // Get the Parent Block
            BlockHeader parent = BlockChain::GetHead()->GetHeader();
            // Sort the Transactions by Timestamp
            //TODO: remove copy
            TransactionList transactions(transactions_);
            std::sort(transactions.begin(), transactions.end(), Transaction::TimestampComparator());
            // Return a New Block of size Block::kMaxTransactionsForBlock, using sorted Transactions
            return new Block(parent, transactions);
        }

        int64_t GetBlockSize() const{
            return size_;
        }

        int64_t GetNumberOfTransactions() const{
            return transactions_.size();
        }

        bool Visit(Transaction* tx){
            if((GetNumberOfTransactions() + 1) >= GetBlockSize())
                return false;
            transactions_.push_back(Transaction(*tx));
            return true;
        }

        static Block* Build(intptr_t size){
            TransactionPoolBlockBuilder builder(size);
            TransactionPool::Accept(&builder);
            Block* block = builder.GetBlock();
            BlockPool::PutBlock(block->GetHash(), block);
            return block;
        }
    };

    static inline void
    OrphanTransaction(Transaction* tx){
        Hash hash = tx->GetHash();
        LOG(WARNING) << "orphaning transaction: " << hash;
        TransactionPool::RemoveTransaction(hash);
    }

    static inline void
    OrphanBlock(Block* blk){
        Hash hash = blk->GetHash();
        LOG(WARNING) << "orphaning block: " << hash;
        BlockPool::RemoveBlock(hash);
    }

    static inline void
    ScheduleNewSnapshot(){
        if(!FLAGS_enable_snapshots) return;
        LOG(INFO) << "scheduling new snapshot....";
        SnapshotTask* task = SnapshotTask::NewInstance();
        if(!task->Submit())
            LOG(WARNING) << "couldn't schedule new snapshot!";
    }

    void BlockDiscoveryThread::HandleVotingPhase(Proposal* proposal){
        LOG(INFO) << "proposal " << proposal << " entering voting phase";
        proposal->SetPhase(Proposal::Phase::kVotingPhase);
        PeerSessionManager::BroadcastPrepare();
        proposal->WaitForRequiredResponses();
        CHECK_STATUS(proposal);
    }

    void BlockDiscoveryThread::HandleCommitPhase(Proposal* proposal){
        LOG(INFO) << "proposal " << proposal << " entering commit phase";
        proposal->SetPhase(Proposal::Phase::kCommitPhase);
        PeerSessionManager::BroadcastCommit();
        proposal->WaitForRequiredResponses();
        CHECK_STATUS(proposal);
    }

    void BlockDiscoveryThread::HandleQuorumPhase(Proposal* proposal){
        LOG(INFO) << "proposal " << proposal << " entering quorum phase";
        proposal->SetPhase(Proposal::Phase::kQuorumPhase);
        CHECK_FOR_REJECTION(proposal);
        LOG(INFO) << "proposal " << proposal << " was accepted";
        proposal->SetStatus(Proposal::Result::kAccepted);
        OnAccepted(proposal);
    }

    void BlockDiscoveryThread::OnAccepted(Proposal* proposal){
        Block* blk = GetBlock();
        DefaultBlockProcessor processor;
        if(!blk->Accept(&processor)){
            LOG(WARNING) << "couldn't process block: " << blk;
            return;
        }
        BlockPool::RemoveBlock(proposal->GetHash());
        BlockChain::Append(blk);
        ScheduleNewSnapshot();
        SetProposal(nullptr);
    }

    void BlockDiscoveryThread::OnRejected(Proposal* proposal){
        //TODO: implement BlockDiscoveryThread::OnRejected(Proposal*)
    }

    Block* BlockDiscoveryThread::CreateNewBlock(intptr_t size){
        Block* blk = TransactionPoolBlockBuilder::Build(size);
        SetBlock(blk);
        return blk;
    }

    Proposal* BlockDiscoveryThread::CreateNewProposal(Block* blk){
        Proposal* proposal = new Proposal(blk, Server::GetID());
        SetProposal(proposal);
        return proposal;
    }

    void BlockDiscoveryThread::SetBlock(Block* blk){
        LOCK_GUARD;
        block_ = blk;
    }

    void BlockDiscoveryThread::SetProposal(Proposal* proposal){
        LOCK_GUARD;
        proposal_ = proposal;
    }

    bool BlockDiscoveryThread::HasProposal(){
        LOCK_GUARD;
        return proposal_ != nullptr;
    }

    Block* BlockDiscoveryThread::GetBlock(){
        LOCK_GUARD;
        return block_;
    }

    Proposal* BlockDiscoveryThread::GetProposal(){
        LOCK_GUARD;
        return proposal_;
    }

    void BlockDiscoveryThread::HandleThread(uword parameter){
        LOG(INFO) << "starting the block discovery thread....";
        SetState(Thread::State::kRunning);
        while(!IsStopped()){
            if(IsPaused()){
                LOG(INFO) << "pausing block discovery thread....";

                LOCK;
                while(GetState() != Thread::State::kRunning || GetState() != Thread::State::kStopped) WAIT;
                UNLOCK;

                if(IsStopped()){
                    LOG(WARNING) << "block discovery thread has been halted, exiting.";
                    return;
                }

                LOG(INFO) << "block discovery thread is resuming.";
            }

            if(HasProposal()){
                Proposal* proposal = GetProposal();
                std::shared_ptr<PeerSession> proposer = proposal->GetPeer();
                Hash hash = proposal->GetHash();

                LOG(INFO) << "proposal " << hash << " has been started by " << proposal->GetProposer();
                if(!BlockPool::HasBlock(hash)){
                    LOG(INFO) << hash << " cannot be found, requesting block from peer: " << proposer->GetID();
                    std::vector<InventoryItem> items = {
                        InventoryItem(InventoryItem::kBlock, hash)
                    };
                    proposer->Send(GetDataMessage::NewInstance(items));
                    LOG(INFO) << "waiting....";
                    BlockPool::WaitForBlock(hash); //TODO: add timeout
                    if(!BlockPool::HasBlock(hash)){
                        LOG(INFO) << "cannot resolve block " << hash << ", rejecting....";
                        proposer->SendRejected();
                        //TODO: abandon proposal
                        return;
                    }
                }
                proposer->SendPromise();
                Block* blk = BlockPool::GetBlock(hash);
                LOG(INFO) << "proposal " << hash << " has entered the voting phase.";
                if(!BlockVerifier::IsValid(blk)){
                    LOG(WARNING) << "cannot validate block " << hash << ", rejecting....";
                    proposer->SendRejected();
                    //TODO: abandon proposal
                    return;
                }

                proposal->WaitForPhase(Proposal::kCommitPhase);
                LOG(INFO) << "proposal " << hash << " has entered the commit phase.";
                if(!BlockChain::Append(blk)){
                    LOG(WARNING) << "couldn't append block " << hash << ", rejecting....";
                    proposer->SendRejected();
                    //TODO: abandon proposal
                    return;
                }
                proposer->SendAccepted();

                proposal->WaitForPhase(Proposal::kQuorumPhase);
                LOG(INFO) << "proposal " << hash << " has finished, " << proposal->GetResult();
            }

            if(GetNumberOfTransactionsInPool() >= 2){
                LOG(INFO) << "creating new block from transaction pool";
                Block* block = CreateNewBlock(2);

                LOG(INFO) << "validating block: " << block;
                if(!BlockVerifier::IsValid(block)){
                    //TODO: orphan block properly
                    LOG(WARNING) << "block " << block << " is invalid, orphaning....";
                    return;
                }

                LOG(INFO) << "discovered block " << block << ", creating proposal....";
                Proposal* proposal = CreateNewProposal(block);
                HandleVotingPhase(proposal);
                CHECK_FOR_REJECTION(proposal);
                HandleCommitPhase(proposal);
                CHECK_FOR_REJECTION(proposal);
                HandleQuorumPhase(proposal);
            }
        }
    }

    Thread::State BlockDiscoveryThread::GetState(){
        LOCK_GUARD;
        return state_;
    }

    void BlockDiscoveryThread::SetState(Thread::State state){
        LOCK_GUARD;
        state_ = state;
    }

    void BlockDiscoveryThread::WaitForState(Thread::State state){
        LOCK;
        while(state_ != state) WAIT;
        UNLOCK;
    }
}