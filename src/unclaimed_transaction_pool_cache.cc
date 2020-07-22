#include "unclaimed_transaction_pool_cache.h"
#include "unclaimed_transaction_pool_index.h"

namespace Token{
    struct uint256_hasher {
        size_t operator()(const uint256_t &hash) const {
            size_t res = 17;
            res = res * 31 + std::hash<std::string>{}(HexString(hash));
            return res;
        }
    };

    static std::unordered_map<uint256_t, Handle<UnclaimedTransaction>, uint256_hasher> map_;
    static std::deque<uint256_t> queue_;

    Handle<UnclaimedTransaction> UnclaimedTransactionPoolCache::GetTransaction(const uint256_t &hash) {
        std::deque<uint256_t>::iterator iter = queue_.begin();
        while ((*iter) != hash) iter++;
#ifdef TOKEN_DEBUG
        LOG(INFO) << "found: " << (*iter);
#endif//TOKEN_DEBUG
        queue_.erase(iter);
        queue_.push_front(hash);
        return map_[hash];
    }

    void UnclaimedTransactionPoolCache::Evict(const uint256_t& hash){
#ifdef TOKEN_DEBUG
        LOG(INFO) << "evicting: " << hash;
#endif//TOKEN_DEBUG
        Handle<UnclaimedTransaction> evicted_tx = map_[hash];
        map_.erase(hash);
        UnclaimedTransactionPoolIndex::PutData(evicted_tx);
    }

    void UnclaimedTransactionPoolCache::Promote(const Token::uint256_t& hash){
        std::deque<uint256_t>::iterator iter = queue_.begin();
        while((*iter) != hash) iter++;
        queue_.erase(iter);
        map_.erase(hash);
    }

    void UnclaimedTransactionPoolCache::EvictLastUsed(){
        uint256_t hash = queue_.back();
        queue_.pop_back();
        Evict(hash);
    }

    void UnclaimedTransactionPoolCache::PutTransaction(const uint256_t& hash, const Handle<UnclaimedTransaction>& tx){
        queue_.push_front(hash);
        map_[hash] = tx;
    }

    size_t UnclaimedTransactionPoolCache::GetSize(){
        return map_.size();
    }

    bool UnclaimedTransactionPoolCache::HasTransaction(const uint256_t& hash){
        return map_.find(hash) != map_.end();
    }
}