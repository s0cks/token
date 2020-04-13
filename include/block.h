#ifndef TOKEN_BLOCK_H
#define TOKEN_BLOCK_H

#include <string>
#include <sstream>
#include <vector>
#include "common.h"
#include "merkle.h"
#include "transaction.h"

namespace Token{
    class BlockVisitor;
    class Block : public BinaryObject{
    private:
        uint64_t timestamp_;
        uint64_t height_;
        uint256_t previous_hash_;
        std::vector<Transaction> transactions_;

        bool GetBytes(CryptoPP::SecByteBlock& bytes) const;

        friend class BlockChain;
    public:
        Block():
            timestamp_(0),
            height_(0),
            previous_hash_(),
            transactions_(){}
        Block(uint64_t height, const uint256_t& previous_hash, const std::vector<Transaction>& transactions, uint64_t timestamp=GetCurrentTime()):
            timestamp_(timestamp),
            height_(height),
            previous_hash_(previous_hash),
            transactions_(transactions){}
        Block(Block* parent, const std::vector<Transaction>& transactions, uint64_t timestamp=GetCurrentTime()):
            timestamp_(timestamp),
            height_(parent->GetHeight() + 1),
            previous_hash_(parent->GetHash()),
            transactions_(transactions){}
        Block(const Proto::BlockChain::Block& raw):
            timestamp_(raw.timestamp()),
            height_(raw.height()),
            previous_hash_(HashFromHexString(raw.previous_hash())),
            transactions_(){
            for(auto& it : raw.transactions()) transactions_.push_back(Transaction(it));
        }
        Block(const Block& other):
            timestamp_(other.timestamp_),
            height_(other.height_),
            previous_hash_(other.previous_hash_),
            transactions_(other.transactions_){}
        ~Block(){}

        MerkleTree* GetMerkleTree() const;

        uint256_t GetPreviousHash() const{
            return previous_hash_;
        }

        uint64_t GetHeight() const{
            return height_;
        }

        uint64_t GetNumberOfTransactions() const{
            return transactions_.size();
        }

        uint64_t GetTimestamp() const{
            return timestamp_;
        }

        Transaction* GetTransaction(uint64_t idx) const{
            if(idx < 0 || idx > GetNumberOfTransactions()) return nullptr;
            return new Transaction(transactions_[idx]);
        }

        Transaction* GetCoinbaseTransaction() const{
            return GetTransaction(0);
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

        friend Proto::BlockChain::Block& operator<<(Proto::BlockChain::Block& stream, const Block& block){
            stream.set_timestamp(block.GetTimestamp());
            stream.set_height(block.GetHeight());
            stream.set_merkle_root(HexString(block.GetMerkleRoot()));
            stream.set_previous_hash(HexString(block.GetPreviousHash()));
            for(size_t idx = 0; idx < block.GetNumberOfTransactions(); idx++){
                Proto::BlockChain::Transaction* raw = stream.add_transactions();
                (*raw) << block.transactions_[idx];
            }
            return stream;
        }

        Block& operator=(const Block& other){
            timestamp_ = other.timestamp_;
            height_ = other.height_;
            previous_hash_ = other.previous_hash_;
            transactions_ = std::vector<Transaction>(other.transactions_);
            return (*this);
        }
    };

    class BlockHeader{
    private:
        uint64_t timestamp_;
        uint64_t height_;
        uint256_t previous_hash_;
        uint256_t hash_;
        uint256_t merkle_root_;
    public:
        BlockHeader(Block* block):
            timestamp_(block->GetTimestamp()),
            height_(block->GetHeight()),
            previous_hash_(block->GetPreviousHash()),
            hash_(block->GetHash()),
            merkle_root_(block->GetMerkleRoot()){}
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
    };

    class BlockVisitor{
    public:
        BlockVisitor(){}
        virtual ~BlockVisitor(){}

        virtual bool VisitBlockStart(){
            return true;
        }

        virtual bool VisitTransaction(Transaction* tx) = 0;

        virtual bool VisitBlockEnd(){
            return true;
        }
    };

    class MerkleTreeBuilder : public BlockVisitor{
    private:
        Block* block_;
        std::vector<uint256_t> leaves_;
        MerkleTree* tree_;
    public:
        MerkleTreeBuilder(const Block* block):
            leaves_(),
            block_(const_cast<Block*>(block)),
            tree_(nullptr){}
        ~MerkleTreeBuilder(){}

        Block* GetBlock() const{
            return block_;
        }

        MerkleTree* GetMerkleTree() const{
            return tree_;
        }

        void AddLeaf(const BinaryObject& bin){
            leaves_.push_back(bin.GetHash());
        }

        bool HasTree() const{
            return tree_ != nullptr;
        }

        bool VisitTransaction(Transaction* tx);
        bool BuildTree();
    };
}

#endif //TOKEN_BLOCK_H