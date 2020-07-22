#include "block_pool_cache.h"
#include "block_pool_index.h"

namespace Token{
    struct uint256_hasher {
        size_t operator()(const uint256_t &hash) const {
            size_t res = 17;
            res = res * 31 + std::hash<std::string>{}(HexString(hash));
            return res;
        }
    };

    static std::unordered_map<uint256_t, Handle<Block>, uint256_hasher> map_;
    static std::deque<uint256_t> queue_;

    Handle<Block> BlockPoolCache::GetTransaction(const uint256_t &hash) {
        std::deque<uint256_t>::iterator iter = queue_.begin();
        while ((*iter) != hash) iter++;
        queue_.erase(iter);
        queue_.push_front(hash);
        return map_[hash];
    }

    void BlockPoolCache::Evict(const uint256_t& hash){
        if(!BlockPoolIndex::HasData(hash)){
            Handle<Block> tx = map_[hash];
            BlockPoolIndex::PutData(tx);
        }
        map_.erase(hash);
    }

    void BlockPoolCache::Promote(const Token::uint256_t& hash){
        std::deque<uint256_t>::iterator iter = queue_.begin();
        while((*iter) != hash) iter++;
        queue_.erase(iter);
        map_.erase(hash);
    }

    void BlockPoolCache::EvictLastUsed(){
        uint256_t hash = queue_.back();
        queue_.pop_back();
        Evict(hash);
    }

    void BlockPoolCache::PutTransaction(const uint256_t& hash, const Handle<Block>& tx){
        queue_.push_front(hash);
        map_[hash] = tx;
    }

    size_t BlockPoolCache::GetSize(){
        return map_.size();
    }

    bool BlockPoolCache::HasTransaction(const uint256_t& hash){
        return map_.find(hash) != map_.end();
    }
}