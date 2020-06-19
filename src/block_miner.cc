#include <glog/logging.h>
#include <algorithm>
#include <memory>

#include "node/node.h"

#include "block_miner.h"
#include "block_chain.h"
#include "block_validator.h"

#include "paxos.h"

namespace Token{
    static pthread_t thread;

    static pthread_mutex_t proposal_mutex_ = PTHREAD_MUTEX_INITIALIZER;
    static pthread_cond_t proposal_cond_ = PTHREAD_COND_INITIALIZER;
    static Proposal* proposal_ = nullptr;

    static uv_timer_t mine_timer_;
    static uv_async_t exit_handle;

    Proposal* BlockMiner::GetProposal(){
        pthread_mutex_lock(&proposal_mutex_);
        Proposal* proposal = proposal_;
        pthread_mutex_unlock(&proposal_mutex_);
        return proposal;
    }

    void BlockMiner::SetProposal(Proposal* proposal){
        pthread_mutex_lock(&proposal_mutex_);
        proposal_ = proposal;
        pthread_mutex_unlock(&proposal_mutex_);
    }

    bool BlockMiner::HasProposal(){
        pthread_mutex_lock(&proposal_mutex_);
        bool found = proposal_ != nullptr;
        pthread_mutex_unlock(&proposal_mutex_);
        return found;
    }

    bool BlockMiner::SubmitProposal(Proposal* proposal){
        NodeInfo info = Node::GetInfo();
        LOG(INFO) << info.GetNodeID() << " creating proposal for: " << proposal->GetHeight() << "....";
        SetProposal(proposal);
        // Node::AsyncBroadcast(new PrepareMessage(info, (*proposal)));

        uint32_t votes_needed = Node::GetNumberOfPeers() > 0 ?
                                (Node::GetNumberOfPeers() == 1 ? 1 : (Node::GetNumberOfPeers() / 2)) :
                                0;

        pthread_mutex_lock(&proposal_mutex_);
        while(proposal->GetNumberOfVotes() < votes_needed){
            pthread_cond_wait(&proposal_cond_, &proposal_mutex_);
        }
        pthread_mutex_unlock(&proposal_mutex_);
        return true;
    }

    bool BlockMiner::CommitProposal(Proposal* proposal){
        NodeInfo info = Node::GetInfo();
        LOG(INFO) << info.GetNodeID() << " committing proposal for: " << proposal->GetHeight() << "....";
        // Node::AsyncBroadcast(new CommitMessage(info, (*proposal)));

        uint32_t commits_needed = Node::GetNumberOfPeers() > 0 ?
                                  (Node::GetNumberOfPeers() == 1 ? 1 : (Node::GetNumberOfPeers() / 2)) :
                                  0;

        pthread_mutex_trylock(&proposal_mutex_);
        while(proposal->GetNumberOfCommits() < commits_needed){
            pthread_cond_wait(&proposal_cond_, &proposal_mutex_);
        }
        pthread_mutex_unlock(&proposal_mutex_);
        return true;
    }

    bool BlockMiner::VoteForProposal(const std::string& node_id){
        Proposal* proposal;
        if(!(proposal = GetProposal())){
            LOG(WARNING) << "couldn't get the proposal";
            return false;
        }

        pthread_mutex_lock(&proposal_mutex_);
        bool voted = proposal->Vote(node_id);
        pthread_cond_signal(&proposal_cond_);
        pthread_mutex_unlock(&proposal_mutex_);
        return voted;
    }

    bool BlockMiner::AcceptProposal(const std::string& node_id){
        Proposal* proposal;
        if(!(proposal = GetProposal())){
            LOG(WARNING) << "couldn't get the proposal";
            return false;
        }

        pthread_mutex_trylock(&proposal_mutex_);
        bool accepted = proposal->Commit(node_id);
        pthread_cond_signal(&proposal_cond_);
        pthread_mutex_unlock(&proposal_mutex_);
        return accepted;
    }

    void BlockMiner::HandleMineCallback(uv_timer_t* handle){
        if(!HasProposal() && TransactionPool::GetSize() >= 2){
            // collect + sort transactions
            std::vector<uint256_t> all_txs;
            if(!TransactionPool::GetTransactions(all_txs)){
                LOG(ERROR) << "couldn't get transactions from transaction pool";
                pthread_mutex_unlock(&proposal_mutex_);
                return;
            }

            std::vector<Transaction*> txs;
            for(auto& it : all_txs){
                //TODO: fixme, terrible design?
                Transaction* tx;
                if(!(tx = TransactionPool::GetTransaction(it))){
                    LOG(WARNING) << "couldn't get transaction: " << it;
                    pthread_mutex_unlock(&proposal_mutex_);
                    return;
                }
                txs.push_back(tx);
            }

            // Create new block for proposal
            BlockHeader head = BlockChain::GetHead();
            LOG(INFO) << "<HEAD> := " << head;

            Block* block = Block::NewInstance(head, txs);
            if(!BlockPool::AddBlock(block)){
                LOG(WARNING) << "couldn't put newly mined block into block pool!";
                return;
            }

            BlockValidator validator(block);
            if(!validator.IsValid()){
                LOG(WARNING) << "blocks isn't valid, orphaning!";
                return;
            }

            // send proposal to peers
            // check proposals for updates
            // append block based on proposals
            // spend inputs, generate unspent outputs

            NodeInfo info = Node::GetInfo();
            uint256_t hash = block->GetHash();
            Proposal* proposal = new Proposal(info.GetNodeID(), block);
            SetProposal(proposal);

            if(!SubmitProposal(proposal)){
                LOG(ERROR) << "couldn't submit proposal for new block: " << hash;
                goto cleanup;
            }

            if(!CommitProposal(proposal)){
                LOG(ERROR) << "couldn't commit proposal for new block: " << hash;
                goto cleanup;
            }

            if(!BlockChain::GetInstance()->Append(block)){
                LOG(ERROR) << "couldn't append new block: " << hash;
                goto cleanup;
            }

            for(uint32_t txid = 0; txid < block->GetNumberOfTransactions(); txid++){
                Transaction* tx;
                if(!(tx = block->GetTransaction(txid))){
                    LOG(WARNING) << "couldn't get transaction #" << txid << " from block: " << hash;
                    return;
                }

                uint256_t tx_hash = tx->GetHash();
                uint32_t idx = 0;
                for(auto it = tx->outputs_begin(); it != tx->outputs_end(); it++){
                    UnclaimedTransaction* utxo = UnclaimedTransaction::NewInstance(tx_hash, idx++);
                    if(!UnclaimedTransactionPool::PutUnclaimedTransaction(utxo)){
                        LOG(WARNING) << "couldn't create unclaimed transaction for: " << tx_hash << "[" << (idx - 1) << "]";
                        continue;
                    }
                }

                for(auto it = tx->inputs_begin(); it != tx->inputs_end(); it++){
                    Transaction* in_tx;
                    if(!(in_tx = BlockChain::GetTransaction((*it)->GetTransactionHash()))){
                        LOG(ERROR) << "couldn't get transaction: " << (*it)->GetTransactionHash();
                        continue;
                    }

                    UnclaimedTransaction* utxo = UnclaimedTransaction::NewInstance(in_tx->GetHash(), (*it)->GetOutputIndex());
                    uint256_t uhash = utxo->GetHash();
                    if(!UnclaimedTransactionPool::RemoveUnclaimedTransaction(uhash)){
                        LOG(ERROR) << "couldn't remove unclaimed transaction: " << uhash;
                        continue;
                    }
                }

                if(!TransactionPool::RemoveTransaction(tx_hash)){
                    LOG(ERROR) << "couldn't remove transaction: " << tx_hash;
                    continue;
                }
            }

            if(!BlockPool::RemoveBlock(hash)){
                LOG(ERROR) << "couldn't remove new block from pool: " << hash;
                goto cleanup;
            }
        cleanup:
            SetProposal(nullptr);
            return;
        }
    }

    void BlockMiner::HandleExitCallback(uv_async_t* handle){
        if(uv_is_closing((uv_handle_t*)handle) == 0){
            uv_close((uv_handle_t*)handle, NULL);
        }
        pthread_exit(nullptr);
    }

    void* BlockMiner::MinerThread(void* data){
        pthread_mutex_init(&proposal_mutex_, NULL);
        pthread_cond_init(&proposal_cond_, NULL);
        uv_loop_t* loop = uv_loop_new();
        uv_async_init(loop, &exit_handle, &HandleExitCallback);
        uv_timer_init(loop, &mine_timer_);
        uv_timer_start(&mine_timer_, &HandleMineCallback, 0, BlockMiner::kMiningIntervalMilliseconds);
        uv_run(loop, UV_RUN_DEFAULT);
        pthread_exit(nullptr);
    }

    bool BlockMiner::Initialize(){
        return pthread_create(&thread, NULL, &BlockMiner::MinerThread, nullptr) == 0;
    }
}