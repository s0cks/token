#ifndef TOKEN_BLOCK_RESOLVER_H
#define TOKEN_BLOCK_RESOLVER_H

#include "blockchain.h"
#include "peer.h"

namespace Token{
    class BlockResolver{
    private:
        std::string target_hash_;
        uint32_t target_height_;
        Block* result_;
    protected:
        bool HasTargetHash() const{
            return !target_hash_.empty();
        }

        bool HasTargetHeight() const{
            return target_height_ >= 0;
        }

        std::string GetTargetHash() const{
            return target_hash_;
        }

        uint32_t GetTargetHeight() const{
            return target_height_;
        }

        virtual bool
        MatchesTarget(Block* block) const{
            if(HasTargetHash()){
                return GetTargetHash() == block->GetHash();
            } else if(HasTargetHeight()){
                return GetTargetHeight() == block->GetHeight();
            }
        }

        inline void
        SetResult(Block* result){
            result_ = result;
        }
    public:
        BlockResolver(const std::string& target):
            target_hash_(target),
            target_height_(-1),
            result_(nullptr){}
        BlockResolver(uint32_t target):
            target_hash_(""),
            target_height_(target),
            result_(nullptr){}
        virtual ~BlockResolver() = default;

        Block* GetResult() const{
            return result_;
        }

        bool HasResult() const{
            return GetResult() != nullptr;
        }

        virtual bool Resolve() = 0;
    };

    class LocalBlockResolver : public BlockResolver{
    public:
        LocalBlockResolver(const std::string& target):
            BlockResolver(target){}
        LocalBlockResolver(uint32_t target):
            BlockResolver(target){}
        ~LocalBlockResolver(){}

        bool Resolve();
    };

    class BlockChainResolver : public BlockResolver{
    public:
        BlockChainResolver(const std::string& target):
            BlockResolver(target){}
        BlockChainResolver(uint32_t target):
            BlockResolver(target){}
        ~BlockChainResolver(){}

        bool Resolve();
    };

    class PeerBlockResolver : public BlockResolver{
    private:
        PeerClient* peer_;

        bool HasBlockNow();
        bool BlockExistsLocally(uint32_t height);
    public:
        PeerBlockResolver(PeerClient* peer, const std::string& target):
            peer_(peer),
            BlockResolver(target){}
        PeerBlockResolver(PeerClient* peer, uint32_t target):
            peer_(peer),
            BlockResolver(target){}
        ~PeerBlockResolver(){}

        PeerClient* GetPeer() const{
            return peer_;
        }

        bool Resolve();
    };
}

#endif //TOKEN_BLOCK_RESOLVER_H