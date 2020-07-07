#include <proposal.h>
#include "alloc/scope.h"
#include "node/task.h"
#include "node/session.h"
#include "node/node.h"

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
        session->Send(VersionMessage::NewInstance(Node::GetID()));
    }

    //TODO:
    // - verify nonce
    void NodeSession::HandleVerackMessage(HandleMessageTask* task){
        VerackMessage* msg = (VerackMessage*)task->GetMessage();
        NodeSession* session = (NodeSession*)task->GetSession();
        NodeAddress paddr = msg->GetCallbackAddress();

        session->Send(VerackMessage::NewInstance(Node::GetID(), NodeAddress("127.0.0.1", FLAGS_port))); //TODO: obtain address dynamically

        session->SetState(NodeSession::kConnected);
        if(!Node::HasPeer(msg->GetID())){
            LOG(WARNING) << "couldn't find peer: " << msg->GetID() << ", connecting to peer " << paddr << "....";
            if(!Node::ConnectTo(paddr)){
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

        Scope scope;
        std::vector<Message*> response;
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

                    Message* data = BlockMessage::NewInstance(block);
                    scope.Retain(data);
                    response.push_back(data);
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

                    Message* data = TransactionMessage::NewInstance(tx);
                    scope.Retain(data);
                    response.push_back(data);
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

        if(Proposal::HasCurrentProposal()){
            session->Send(RejectedMessage::NewInstance(proposal));
            return;
        }

        Proposal::SetCurrentProposal(proposal);
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
        uint256_t hash = block->GetHash();

        BlockPool::PutBlock(block);
        //TODO: session->OnHash(hash); - move to block chain class, block calling thread
    }

    void NodeSession::HandleTransactionMessage(HandleMessageTask* task){
        NodeSession* session = (NodeSession*)task->GetSession();
        TransactionMessage* msg = (TransactionMessage*)task->GetMessage();

        Transaction* tx = msg->GetTransaction();
        TransactionPool::PutTransaction(tx);
        //TODO: session->OnHash(tx->GetHash());
    }

    void NodeSession::HandleAcceptedMessage(HandleMessageTask* task){}

    void NodeSession::HandleRejectedMessage(HandleMessageTask* task){}

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
}