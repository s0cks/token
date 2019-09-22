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
        if(HasTargetHash()){
            for(int i = 0;
                i < BlockChain::GetInstance()->GetHeight();
                i++){
                Block* tmp = BlockChain::GetInstance()->LoadBlock(i);
                if(!tmp){
                    LOG(ERROR) << "couldn't load block: @" << i;
                    return false;
                }

                if(GetTargetHash() != tmp->GetHash()){
                    delete tmp;
                    continue;
                }

                SetResult(tmp);
                return true;
            }
        } else if(HasTargetHeight()){
            SetResult(BlockChain::GetInstance()->LoadBlock(GetTargetHeight()));
            return HasResult();
        } else{
            LOG(ERROR) << "unknown target type";
            return false;
        }
    }

    bool BlockChainResolver::Visit(Token::Block* block){
        if(HasTargetHeight()){
            if(GetTargetHeight() == block->GetHeight()){
                SetResult(block);
                return true;
            }
        } else if(HasTargetHash()){
            if(GetTargetHash() == block->GetHash()){
                SetResult(block);
                return true;
            }
        }
        return false;
    }

    bool BlockChainResolver::Resolve(){
        BlockChain::Accept(this);
        return HasResult();
    }

    bool PeerBlockResolver::BlockExistsLocally(uint32_t height){
        std::string filename;
        //TODO: std::string filename = BlockChain::GetInstance()->GetBlockLocation(height);
        return FileExists(filename);
    }

    bool PeerBlockResolver::HasBlockNow(){
        if(HasTargetHeight()){
            LocalBlockResolver resolver(GetTargetHeight());
            resolver.Resolve();
            SetResult(resolver.GetResult());
            return resolver.HasResult();
        } else if(HasTargetHash()){
            return false;
        } else{
            LOG(ERROR) << "unknown target type";
            return false;
        }
    }

    bool PeerBlockResolver::Resolve(){
        Node::Messages::GetBlockRequest getblockreq;
        if(HasTargetHash()){
            getblockreq.set_hash(GetTargetHash());
        } else if(HasTargetHeight()){
            getblockreq.set_height(GetTargetHeight());
        } else{
            LOG(ERROR) << "unknown target type";
            return false;
        }
        Message getblock(Message::Type::kGetBlockMessage, &getblockreq);
        GetPeer()->AsyncSend(&getblock);
        sleep(5);
        return HasBlockNow();
    }
}