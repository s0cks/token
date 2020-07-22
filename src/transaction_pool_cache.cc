#include <deque>
#include <unordered_map>
#include <glog/logging.h>
#include "transaction_pool_cache.h"
#include "transaction_pool_index.h"

namespace Token {
    struct uint256_hasher {
        size_t operator()(const uint256_t &hash) const {
            size_t res = 17;
            res = res * 31 + std::hash<std::string>{}(HexString(hash));
            return res;
        }
    };

    static std::unordered_map<uint256_t, Handle<Transaction>, uint256_hasher> map_;
    static std::deque<uint256_t> queue_;

    Handle<Transaction> TransactionPoolCache::GetTransaction(const uint256_t &hash) {
        std::deque<uint256_t>::iterator iter = queue_.begin();
        while ((*iter) != hash) iter++;
#ifdef TOKEN_DEBUG
        LOG(INFO) << "found: " << (*iter);
#endif//TOKEN_DEBUG
        queue_.erase(iter);
        queue_.push_front(hash);
        return map_[hash];
    }

    void TransactionPoolCache::Evict(const uint256_t& hash){
#ifdef TOKEN_DEBUG
        LOG(INFO) << "evicting: " << hash;
#endif//TOKEN_DEBUG
        Handle<Transaction> evicted_tx = map_[hash];
        map_.erase(hash);
        TransactionPoolIndex::PutData(evicted_tx);
    }

    void TransactionPoolCache::Promote(const Token::uint256_t& hash){
        std::deque<uint256_t>::iterator iter = queue_.begin();
        while((*iter) != hash) iter++;
        queue_.erase(iter);
        map_.erase(hash);
    }

    void TransactionPoolCache::EvictLastUsed(){
        uint256_t hash = queue_.back();
        queue_.pop_back();
        Evict(hash);
    }

    void TransactionPoolCache::PutTransaction(const uint256_t& hash, const Handle<Transaction>& tx){
        queue_.push_front(hash);
        map_[hash] = tx;
    }

    size_t TransactionPoolCache::GetSize(){
        return map_.size();
    }

    bool TransactionPoolCache::HasTransaction(const uint256_t& hash){
        return map_.find(hash) != map_.end();
    }
}