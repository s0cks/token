#include <unistd.h>
#include <glog/logging.h>
#include "block_resolver.h"

namespace Token{
    static inline bool
    FileExists(const std::string& name){
        std::ifstream f(name.c_str());
        return f.good();
    }

    bool LocalBlockResolver::Resolve(){
        uint32_t height;
        if(HasTargetHash()){
            height = BlockChain::GetInstance()->GetBlockHeightFromHash(GetTargetHash());
        } else if(HasTargetHeight()){
            height = GetTargetHeight();
        } else{
            LOG(ERROR) << "unknown target type";
            return false;
        }
        std::string blkfile = BlockChain::GetInstance()->GetBlockDataFile(height);
        return FileExists(blkfile);
    }

    bool BlockChainResolver::Visit(Token::Block* block){
        LOG(INFO) << "visiting " << block->GetHash();
        if(MatchesTarget(block)){
            SetResult(block);
            return false; // don't continue
        }
        return true;
    }

    bool BlockChainResolver::Resolve(){
        BlockChain::GetInstance()->Accept(this);
        return HasResult();
    }

    bool PeerBlockResolver::BlockExistsLocally(uint32_t height){
        std::string filename = BlockChain::GetInstance()->GetBlockDataFile(height);
        return FileExists(filename);
    }

    bool PeerBlockResolver::HasBlockNow(){
        uint32_t height;
        if(HasTargetHash()){
            height = BlockChain::GetInstance()->GetBlockHeightFromHash(GetTargetHash());
        } else if(HasTargetHeight()){
            height = GetTargetHeight();
        } else{
            LOG(ERROR) << "invalid target type";
            return false;
        }
        return BlockExistsLocally(height);
    }

    bool PeerBlockResolver::Resolve(){
        Node::Messages::GetBlockRequest getblockreq;
        if(HasTargetHash()){
            getblockreq.set_hash(GetTargetHash());
        } else if(HasTargetHeight()){
            //TODO: getblockreq.set_height(GetTargetHeight());
        } else{
            LOG(ERROR) << "unknown target type";
            return false;
        }
        Message getblock(Message::Type::kGetBlockMessage, &getblockreq);
        GetPeer()->AsyncSend(&getblock);
        sleep(2);
        return HasBlockNow();
    }
}