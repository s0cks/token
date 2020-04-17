#ifndef TOKEN_BLOCK_H
#define TOKEN_BLOCK_H

#include <string>
#include <sstream>
#include <vector>
#include <map>
#include "blockchain.pb.h"
#include "service.pb.h" //TODO: remove
#include "common.h"
#include "merkle.h"
#include "transaction.h"

namespace Token{
    class BlockHeader{
    private:
        uint64_t timestamp_;
        uint64_t height_;
        uint256_t previous_hash_;
        uint256_t hash_;
        uint256_t merkle_root_;
    public:
        BlockHeader():
            timestamp_(0),
            height_(0),
            previous_hash_(),
            hash_(),
            merkle_root_(){}
        BlockHeader(const Block& block);
        ~BlockHeader(){}

        uint64_t GetTimestamp() const{
            return timestamp_;
        }

        uint64_t GetHeight() const{
            return height_;
        }

        uint256_t GetPreviousHash() const{
            return previous_hash_;
        }

        uint256_t GetHash() const{
            return hash_;
        }

        uint256_t GetMerkleRoot() const{
            return merkle_root_;
        }

        Block* GetData() const;

        friend bool operator==(const BlockHeader& a, const BlockHeader& b){
            return a.GetHash() == b.GetHash();
        }

        friend bool operator!=(const BlockHeader& a, const BlockHeader& b){
            return !operator==(a, b);
        }

        BlockHeader& operator=(const BlockHeader& other){
            timestamp_ = other.timestamp_;
            height_ = other.height_;
            previous_hash_ = other.previous_hash_;
            hash_ = other.hash_;
            merkle_root_ = other.merkle_root_;
            return (*this);
        }

        friend Proto::BlockChain::BlockHeader& operator<<(Proto::BlockChain::BlockHeader& stream, const BlockHeader& block){
            stream.set_timestamp(block.GetTimestamp());
            stream.set_height(block.GetHeight());
            stream.set_merkle_root(HexString(block.GetMerkleRoot()));
            stream.set_hash(HexString(block.GetHash()));
            stream.set_previous_hash(HexString(block.GetPreviousHash()));
            return stream;
        }
    };

    class BlockVisitor;
    class Block : public BinaryObject{
    private:
        typedef std::map<uint256_t, Transaction> TransactionMap;
        typedef std::pair<uint256_t, Transaction> TransactionMapPair;
        typedef std::vector<Transaction> TransactionList;

        uint64_t timestamp_;
        uint64_t height_;
        uint256_t previous_hash_;
        TransactionList tx_list_;
        TransactionMap tx_map_;

        bool GetBytes(CryptoPP::SecByteBlock& bytes) const;

        friend class BlockChain;
    public:
        typedef Proto::BlockChain::Block RawType;

        Block():
            timestamp_(0),
            height_(0),
            previous_hash_(),
            tx_list_(),
            tx_map_(){}
        Block(uint64_t height, const uint256_t& previous_hash, const std::vector<Transaction>& transactions, uint64_t timestamp=GetCurrentTime()):
            timestamp_(timestamp),
            height_(height),
            previous_hash_(previous_hash),
            tx_list_(transactions),
            tx_map_(){
            for(auto& it : transactions) tx_map_.insert(TransactionMapPair(it.GetHash(), it));
        }
        Block(const BlockHeader& parent, const std::vector<Transaction>& transactions, uint64_t timestamp=GetCurrentTime()):
            timestamp_(timestamp),
            height_(parent.GetHeight() + 1),
            previous_hash_(parent.GetHash()),
            tx_list_(transactions),
            tx_map_(){
            for(auto& it : transactions) tx_map_.insert(TransactionMapPair(it.GetHash(), it));
        }
        Block(Block* parent, const std::vector<Transaction>& transactions, uint64_t timestamp=GetCurrentTime()):
            timestamp_(timestamp),
            height_(parent->GetHeight() + 1),
            previous_hash_(parent->GetHash()),
            tx_list_(transactions),
            tx_map_(){
            for(auto& it : transactions) tx_map_.insert(TransactionMapPair(it.GetHash(), it));
        }
        Block(const Proto::BlockChain::Block& raw):
            timestamp_(raw.timestamp()),
            height_(raw.height()),
            previous_hash_(HashFromHexString(raw.previous_hash())),
            tx_list_(),
            tx_map_(){
            for(auto& it : raw.transactions()){
                Transaction tx(it);
                tx_list_.push_back(it);
                tx_map_.insert(TransactionMapPair(tx.GetHash(), tx));
            }
        }
        Block(const Block& other):
            timestamp_(other.timestamp_),
            height_(other.height_),
            previous_hash_(other.previous_hash_),
            tx_list_(other.tx_list_),
            tx_map_(other.tx_map_){}
        ~Block(){}

        MerkleTree* GetMerkleTree() const;

        BlockHeader GetHeader() const{
            return BlockHeader((*this));
        }

        uint256_t GetPreviousHash() const{
            return previous_hash_;
        }

        uint64_t GetHeight() const{
            return height_;
        }

        uint64_t GetNumberOfTransactions() const{
            return tx_list_.size();
        }

        uint64_t GetTimestamp() const{
            return timestamp_;
        }

        bool GetTransaction(uint64_t idx, Transaction* result) const{
            if(idx < 0 || idx > GetNumberOfTransactions()) return false;
            (*result) = tx_list_[idx];
            return true;
        }

        bool GetTransaction(const uint256_t& hash, Transaction* result) const{
            auto pos = tx_map_.find(hash);
            if(pos == tx_map_.end()) return false;
            (*result) = pos->second;
            return true;
        }

        bool IsGenesis(){
            return GetHeight() == 0;
        }

        bool Contains(const uint256_t& hash) const;
        bool Accept(BlockVisitor* vis) const;
        uint256_t GetMerkleRoot() const;

        friend bool operator==(const Block& lhs, const Block& rhs){
            return const_cast<Block&>(lhs).GetHash() == const_cast<Block&>(rhs).GetHash();
        }

        friend bool operator!=(const Block& lhs, const Block& rhs){
            return !operator==(lhs, rhs);
        }

        friend RawType& operator<<(RawType& stream, const Block& block){
            stream.set_timestamp(block.GetTimestamp());
            stream.set_height(block.GetHeight());
            stream.set_merkle_root(HexString(block.GetMerkleRoot()));
            stream.set_previous_hash(HexString(block.GetPreviousHash()));
            for(size_t idx = 0; idx < block.GetNumberOfTransactions(); idx++){
                Proto::BlockChain::Transaction* raw = stream.add_transactions();
                (*raw) << block.tx_list_[idx];
            }
            return stream;
        }

        Block& operator=(const Block& other){
            timestamp_ = other.timestamp_;
            height_ = other.height_;
            previous_hash_ = other.previous_hash_;
            tx_list_ = other.tx_list_;
            tx_map_ = other.tx_map_;
            return (*this);
        }
    };

    class BlockVisitor{
    public:
        BlockVisitor(){}
        virtual ~BlockVisitor(){}

        virtual bool VisitStart(){ return true; }
        virtual bool Visit(const Transaction& tx) = 0;
        virtual bool VisitEnd(){ return true; }
    };
}

#endif //TOKEN_BLOCK_H