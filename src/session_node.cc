#include "task.h"
#include "server.h"
#include "session.h"
#include "proposal.h"
#include "proposer.h"
#include "block_pool.h"
#include "transaction_pool.h"
#include "unclaimed_transaction_pool.h"

namespace Token{
    void NodeSession::HandleNotFoundMessage(const Handle<HandleMessageTask>& task){
        //TODO: implement HandleNotFoundMessage
        LOG(WARNING) << "not implemented";
        return;
    }

    void NodeSession::HandleVersionMessage(const Handle<HandleMessageTask>& task){
        NodeSession* session = (NodeSession*)task->GetSession();
        //TODO:
        // - state check
        // - version check
        // - echo nonce
        session->Send(VersionMessage::NewInstance(Server::GetID()));
    }

    //TODO:
    // - verify nonce
    void NodeSession::HandleVerackMessage(const Handle<HandleMessageTask>& task){
        NodeSession* session = (NodeSession*)task->GetSession();
        Handle<VerackMessage> msg = task->GetMessage().CastTo<VerackMessage>();

        NodeAddress callback("127.0.0.1", FLAGS_port); //TODO: obtain address dynamically
        session->Send(VerackMessage::NewInstance(ClientType::kNode, Server::GetID(), callback));

        if(session->IsConnecting()){
            session->SetState(Session::State::kConnected);
            if(msg->IsNode()){
                NodeAddress paddr = msg->GetCallbackAddress();
                if(!Server::HasPeer(callback)){
                    LOG(WARNING) << "couldn't find peer: " << msg->GetID() << ", connecting to peer " << paddr << "....";
                    if(!Server::ConnectTo(paddr)){
                        LOG(WARNING) << "couldn't connect to peer: " << paddr;
                    }
                }
            }
        }
    }

    void NodeSession::HandleGetDataMessage(const Handle<HandleMessageTask>& task){
        NodeSession* session = (NodeSession*)task->GetSession();
        Handle<GetDataMessage> msg = task->GetMessage().CastTo<GetDataMessage>();

        std::vector<InventoryItem> items;
        if(!msg->GetItems(items)){
            LOG(WARNING) << "cannot get items from message";
            return;
        }

        std::vector<Handle<Message>> response;
        for(auto& item : items){
            uint256_t hash = item.GetHash();
            if(item.ItemExists()){
                if(item.IsBlock()){
                    Block* block = nullptr;
                    if(BlockChain::HasBlock(hash)){
                        block = BlockChain::GetBlockData(hash);
                    } else if(BlockPool::HasBlock(hash)){
                        block = BlockPool::GetBlock(hash);
                    } else{
                        //TODO: return 404
#ifdef TOKEN_DEBUG
                        LOG(WARNING) << "cannot find block: " << hash;
#endif//TOKEN_DEBUG
                        return;
                    }
                    response.push_back(BlockMessage::NewInstance(block).CastTo<Message>());
                } else if(item.IsTransaction()){
                    Transaction* tx = nullptr;
                    if(TransactionPool::HasTransaction(hash)){
                        tx = TransactionPool::GetTransaction(hash);
                    } else{
                        //TODO: return 404
#ifdef TOKEN_DEBUG
                        LOG(WARNING) << "couldn't find transaction: " << hash;
#endif//TOKEN_DEBUG
                        return;
                    }
                    response.push_back(TransactionMessage::NewInstance(tx).CastTo<Message>());
                }
            } else{
                //TODO: return 500
#ifdef TOKEN_DEBUG
                LOG(WARNING) << "item is invalid: " << item;
#endif//TOKEN_DEBUG
            }
        }

        session->Send(response);
    }

    void NodeSession::HandlePrepareMessage(const Handle<HandleMessageTask>& task){
        NodeSession* session = (NodeSession*)task->GetSession();
        Handle<PrepareMessage> msg = task->GetMessage().CastTo<PrepareMessage>();

        Handle<Proposal> proposal = msg->GetProposal();
        if(ProposerThread::HasProposal()){
            session->Send(RejectedMessage::NewInstance(proposal));
            return;
        }

        ProposerThread::SetProposal(proposal);
        proposal->SetPhase(Proposal::kVotingPhase);
        std::vector<Handle<Message>> response;
        if(!BlockPool::HasBlock(proposal->GetHash())){
            std::vector<InventoryItem> items = {
                InventoryItem(InventoryItem::kBlock, proposal->GetHash())
            };
            response.push_back(GetDataMessage::NewInstance(items).CastTo<Message>());
        }
        response.push_back(PromiseMessage::NewInstance(proposal).CastTo<Message>());
        session->Send(response);
    }

    void NodeSession::HandlePromiseMessage(const Handle<HandleMessageTask>& task){}

    void NodeSession::HandleCommitMessage(const Handle<HandleMessageTask>& task){
        NodeSession* session = (NodeSession*)task->GetSession();
        Handle<CommitMessage> msg = task->GetMessage().CastTo<CommitMessage>();

        Proposal* proposal = msg->GetProposal();
        uint256_t hash = proposal->GetHash();

        if(!BlockPool::HasBlock(hash)){
            LOG(WARNING) << "couldn't find block " << hash << " in pool, rejecting request to commit proposal....";
            session->Send(RejectedMessage::NewInstance(proposal));
        } else{
            proposal->SetPhase(Proposal::kCommitPhase);
            session->Send(AcceptedMessage::NewInstance(proposal));
        }
    }

    void NodeSession::HandleAcceptedMessage(const Handle<HandleMessageTask>& task){
        NodeSession* session = (NodeSession*)task->GetSession();
        Handle<AcceptedMessage> msg = task->GetMessage().CastTo<AcceptedMessage>();
        Handle<Proposal> proposal = msg->GetProposal();

        // validate proposal still?
        proposal->SetPhase(Proposal::kQuorumPhase);
    }

    void NodeSession::HandleRejectedMessage(const Handle<HandleMessageTask>& task){}

    void NodeSession::HandleBlockMessage(const Handle<HandleMessageTask>& task){
        NodeSession* session = (NodeSession*)task->GetSession();
        Handle<BlockMessage> msg = task->GetMessage().CastTo<BlockMessage>();

        Block* block = msg->GetBlock();
        uint256_t hash = block->GetSHA256Hash();

        if(!BlockPool::HasBlock(hash)){
            BlockPool::PutBlock(block);
            Server::Broadcast(InventoryMessage::NewInstance(block).CastTo<Message>());
        }
    }

    void NodeSession::HandleTransactionMessage(const Handle<HandleMessageTask>& task){
        NodeSession* session = (NodeSession*)task->GetSession();
        Handle<TransactionMessage> msg = task->GetMessage().CastTo<TransactionMessage>();

        Transaction* tx = msg->GetTransaction();
        uint256_t hash = tx->GetHash();

#ifdef TOKEN_DEBUG
        LOG(INFO) << "received transaction: " << hash;
#endif//TOKEN_DEBUG

        if(!TransactionPool::HasTransaction(hash)){
            TransactionPool::PutTransaction(tx);
            Server::Broadcast(InventoryMessage::NewInstance(tx).CastTo<Message>());
        }
    }

    void NodeSession::HandleInventoryMessage(const Handle<HandleMessageTask>& task){
        NodeSession* session = (NodeSession*)task->GetSession();
        Handle<InventoryMessage> msg = task->GetMessage().CastTo<InventoryMessage>();

        std::vector<InventoryItem> items;
        if(!msg->GetItems(items)){
            LOG(WARNING) << "couldn't get items from inventory message";
            return;
        }

        std::vector<InventoryItem> needed;
        for(auto& item : items){
            if(!item.ItemExists()) needed.push_back(item);
        }

        LOG(INFO) << "downloading " << needed.size() << "/" << items.size() << " items from inventory....";
        if(!needed.empty()) session->Send(GetDataMessage::NewInstance(needed));
    }

    class UserUnclaimedTransactionCollector : public UnclaimedTransactionPoolVisitor{
    private:
        std::vector<InventoryItem>& items_;
        std::string target_;
    public:
        UserUnclaimedTransactionCollector(std::vector<InventoryItem>& items, const std::string& target):
            UnclaimedTransactionPoolVisitor(),
            items_(items),
            target_(target){}
        ~UserUnclaimedTransactionCollector(){}

        std::string GetTarget() const{
            return target_;
        }

        bool Visit(const Handle<UnclaimedTransaction>& utxo){
            if(utxo->GetUser() == GetTarget()) items_.push_back(InventoryItem(utxo));
            return true;
        }
    };

    void NodeSession::HandleGetUnclaimedTransactionsMessage(const Token::Handle<Token::HandleMessageTask>& task){
        NodeSession* session = (NodeSession*)task->GetSession();
        Handle<GetUnclaimedTransactionsMessage> msg = task->GetMessage().CastTo<GetUnclaimedTransactionsMessage>();

        std::string user = msg->GetUser();
        LOG(INFO) << "getting unclaimed transactions for " << user << "....";

        std::vector<InventoryItem> items;
        UserUnclaimedTransactionCollector collector(items, user);
        if(!UnclaimedTransactionPool::Accept(&collector)){
            LOG(WARNING) << "couldn't visit unclaimed transaction pool";
            //TODO: send not found?
            return;
        }

        if(items.empty()){
            LOG(WARNING) << "no unclaimed transactions found for user: " << user;
            //TODO: send not found?
            return;
        }

        LOG(INFO) << "sending inventory of " << items.size() << " items";
        session->SendInventory(items);
    }

    void NodeSession::HandleGetBlocksMessage(const Handle<HandleMessageTask>& task){
        NodeSession* session = (NodeSession*)task->GetSession();
        Handle<GetBlocksMessage> msg = task->GetMessage().CastTo<GetBlocksMessage>();

        uint256_t start = msg->GetHeadHash();
        uint256_t stop = msg->GetStopHash();

        std::vector<InventoryItem> items;
        if(stop.IsNull()){
            uint32_t amt = std::min(GetBlocksMessage::kMaxNumberOfBlocks, BlockChain::GetHead().GetHeight());
            LOG(INFO) << "sending " << (amt + 1) << " blocks...";

            BlockHeader start_block = BlockChain::GetBlock(start);
            BlockHeader stop_block = BlockChain::GetBlock(start_block.GetHeight() > amt ? start_block.GetHeight() + amt : amt);

            for(uint32_t idx = start_block.GetHeight() + 1;
                idx <= stop_block.GetHeight();
                idx++){
                BlockHeader block = BlockChain::GetBlock(idx);
                LOG(INFO) << "adding " << block;
                items.push_back(InventoryItem(block));
            }
        }

        session->Send(InventoryMessage::NewInstance(items));
    }
}