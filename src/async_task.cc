#include "async_task.h"
#include "block_handler.h"
#include "proposal.h"

namespace Token{
    bool SynchronizeBlockChainTask::ProcessBlock(const Handle<Block>& block){
        BlockHeader header = block->GetHeader();
        if(!BlockHandler::ProcessBlock(block, false)){
            LOG(WARNING) << "couldn't process block: " << header;
            return false;
        }
        BlockPool::RemoveBlock(header.GetHash());
        BlockChain::Append(block);
        return true;
    }

    Result SynchronizeBlockChainTask::DoWork(){
        if(!BlockPool::HasBlock(head_)){
            LOG(INFO) << "waiting for new <HEAD>: " << head_;
            GetSession()->WaitForItem(GetHead());
        }

        std::deque<uint256_t> work; // we are queuing the blocks just in-case there is an unresolved previous hash
        work.push_back(head_);
        do{
            uint256_t hash = work.front();
            work.pop_front();

            if(!BlockPool::HasBlock(hash)){
                LOG(INFO) << "waiting for: " << hash;
                GetSession()->WaitForItem(GetItem(hash));
            }

            Handle<Block> blk = BlockPool::GetBlock(hash);
            uint256_t phash = blk->GetPreviousHash();
            if(!BlockChain::HasBlock(phash)){
                LOG(WARNING) << "parent block " << phash << " not found, resolving...";
                work.push_front(hash);
                work.push_front(phash);
                continue;
            }

            if(!ProcessBlock(blk)){
                std::stringstream ss;
                ss << "couldn't process block: " << hash;
                return Result(Result::kFailed, ss.str());
            }
        } while(!work.empty());
        return Result(Result::kSuccessful, "synchronized block chain");
    }
}