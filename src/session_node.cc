#include <proposal.h>
#include "task.h"
#include "session.h"
#include "server.h"
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

        NodeAddress paddr = msg->GetCallbackAddress();
        session->Send(VerackMessage::NewInstance(Server::GetID())); //TODO: obtain address dynamically

        session->SetState(NodeSession::kConnected);
        if(!Server::HasPeer(msg->GetID())){
            LOG(WARNING) << "couldn't find peer: " << msg->GetID() << ", connecting to peer " << paddr << "....";
            if(!Server::ConnectTo(paddr)){
                LOG(WARNING) << "couldn't connect to peer: " << paddr;
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

        Proposal* proposal = msg->GetProposal();
        //TODO: validate proposal first

        if(BlockMiner::HasProposal()){
            session->Send(RejectedMessage::NewInstance(proposal));
            return;
        }

        BlockMiner::SetProposal(proposal);
        session->Send(PromiseMessage::NewInstance(proposal));
    }

    void NodeSession::HandlePromiseMessage(const Handle<HandleMessageTask>& task){}

    void NodeSession::HandleCommitMessage(const Handle<HandleMessageTask>& task){
        NodeSession* session = (NodeSession*)task->GetSession();
        Handle<CommitMessage> msg = task->GetMessage().CastTo<CommitMessage>();

        Proposal* proposal = msg->GetProposal();
        uint256_t hash = proposal->GetHash();

        Block* block;
        if(!(block = BlockPool::GetBlock(hash))){
            LOG(WARNING) << "couldn't find block " << hash << " from pool waiting....";
            goto exit;
        }

        BlockPool::RemoveBlock(hash);

        session->Send(AcceptedMessage::NewInstance(proposal));
    exit:
        return;
    }

    void NodeSession::HandleBlockMessage(const Handle<HandleMessageTask>& task){
        NodeSession* session = (NodeSession*)task->GetSession();
        Handle<BlockMessage> msg = task->GetMessage().CastTo<BlockMessage>();

        Block* block = msg->GetBlock();
        uint256_t hash = block->GetSHA256Hash();

        BlockPool::PutBlock(block);
        //TODO: session->OnHash(hash); - move to block chain class, block calling thread
    }

    void NodeSession::HandleTransactionMessage(const Handle<HandleMessageTask>& task){
        NodeSession* session = (NodeSession*)task->GetSession();
        Handle<TransactionMessage> msg = task->GetMessage().CastTo<TransactionMessage>();

        Transaction* tx = msg->GetTransaction();
        uint256_t hash = tx->GetSHA256Hash();

#ifdef TOKEN_DEBUG
        LOG(INFO) << "received transaction: " << hash;
#endif//TOKEN_DEBUG

        if(!TransactionPool::HasTransaction(hash)){
            TransactionPool::PutTransaction(tx);
            Server::Broadcast(InventoryMessage::NewInstance(tx).CastTo<Message>());
        }
    }

    void NodeSession::HandleAcceptedMessage(const Handle<HandleMessageTask>& task){}
    void NodeSession::HandleRejectedMessage(const Handle<HandleMessageTask>& task){}

    void NodeSession::HandleTestMessage(const Handle<HandleMessageTask>& task){
        NodeSession* session = (NodeSession*)task->GetSession();
        Handle<TestMessage> msg = task->GetMessage().CastTo<TestMessage>();
        LOG(INFO) << "Test: " << msg->GetHash();
    }

    void NodeSession::HandleInventoryMessage(const Handle<HandleMessageTask>& task){
        NodeSession* session = (NodeSession*)task->GetSession();
        Handle<InventoryMessage> msg = task->GetMessage().CastTo<InventoryMessage>();

        std::vector<InventoryItem> items;
        if(!msg->GetItems(items)){
            LOG(WARNING) << "couldn't get items from inventory message";
            return;
        }

        auto item_exists = [](InventoryItem item){
            return item.ItemExists();
        };
        items.erase(std::remove_if(items.begin(), items.end(), item_exists), items.end());

        for(auto& item : items) LOG(INFO) << "downloading " << item << "....";
        if(!items.empty()) session->Send(GetDataMessage::NewInstance(items));
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

    void NodeSession::HandlePingMessage(const Handle<HandleMessageTask>& task){}
    void NodeSession::HandlePongMessage(const Handle<HandleMessageTask>& task){}
}