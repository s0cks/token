#include <proposal.h>
#include "task.h"
#include "session.h"
#include "server.h"
#include "block_pool.h"
#include "transaction_pool.h"
#include "unclaimed_transaction_pool.h"

namespace Token{
    void NodeSession::HandleNotFoundMessage(Token::HandleMessageTask* task){
        //TODO: implement HandleNotFoundMessage
        LOG(WARNING) << "not implemented";
        return;
    }

    void NodeSession::HandleVersionMessage(HandleMessageTask* task){
        NodeSession* session = (NodeSession*)task->GetSession();
        //TODO:
        // - state check
        // - version check
        // - echo nonce
        session->Send(VersionMessage::NewInstance(Server::GetID()));
    }

    //TODO:
    // - verify nonce
    void NodeSession::HandleVerackMessage(HandleMessageTask* task){
        VerackMessage* msg = (VerackMessage*)task->GetMessage();
        NodeSession* session = (NodeSession*)task->GetSession();
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

    void NodeSession::HandleGetDataMessage(HandleMessageTask* task){
        NodeSession* session = (NodeSession*)task->GetSession();
        GetDataMessage* msg = (GetDataMessage*)task->GetMessage();

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

    void NodeSession::HandlePrepareMessage(HandleMessageTask* task){
        NodeSession* session = (NodeSession*)task->GetSession();
        PrepareMessage* msg = (PrepareMessage*)task->GetMessage();

        Proposal* proposal = msg->GetProposal();
        //TODO: validate proposal first

        if(BlockMiner::HasProposal()){
            session->Send(RejectedMessage::NewInstance(proposal));
            return;
        }

        BlockMiner::SetProposal(proposal);
        session->Send(PromiseMessage::NewInstance(proposal));
    }

    void NodeSession::HandlePromiseMessage(HandleMessageTask* task){}

    void NodeSession::HandleCommitMessage(HandleMessageTask* task){
        NodeSession* session = (NodeSession*)task->GetSession();
        CommitMessage* msg = (CommitMessage*)task->GetMessage();

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

    void NodeSession::HandleBlockMessage(HandleMessageTask* task){
        NodeSession* session = (NodeSession*)task->GetSession();
        BlockMessage* msg = (BlockMessage*)task->GetMessage();

        Block* block = msg->GetBlock();
        uint256_t hash = block->GetSHA256Hash();

        BlockPool::PutBlock(block);
        //TODO: session->OnHash(hash); - move to block chain class, block calling thread
    }

    void NodeSession::HandleTransactionMessage(HandleMessageTask* task){
        NodeSession* session = (NodeSession*)task->GetSession();
        TransactionMessage* msg = (TransactionMessage*)task->GetMessage();

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

    void NodeSession::HandleAcceptedMessage(HandleMessageTask* task){}
    void NodeSession::HandleRejectedMessage(HandleMessageTask* task){}

    void NodeSession::HandleTestMessage(HandleMessageTask* task){
        NodeSession* session = (NodeSession*)task->GetSession();
        TestMessage* msg = (TestMessage*)task->GetMessage();
        LOG(INFO) << "Test: " << msg->GetHash();
    }

    void NodeSession::HandleInventoryMessage(HandleMessageTask* task){
        NodeSession* session = (NodeSession*)task->GetSession();
        InventoryMessage* msg = (InventoryMessage*)task->GetMessage();

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

    void NodeSession::HandleGetBlocksMessage(HandleMessageTask* task){
        NodeSession* session = (NodeSession*)task->GetSession();
        GetBlocksMessage* msg = (GetBlocksMessage*)task->GetMessage();

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

    void NodeSession::HandlePingMessage(HandleMessageTask* task){}
    void NodeSession::HandlePongMessage(HandleMessageTask* task){}
}