#ifndef TOKEN_BLOCK_H
#define TOKEN_BLOCK_H

#include <string>
#include <sstream>

#include <vector>
#include "merkle.h"
#include "blockchain.pb.h"
#include "transaction.h"

namespace Token{
    class BlockVisitor;
    class Block{
    private:
        uint64_t height_;
        std::string previous_hash_;
        std::vector<Transaction*> transactions_;

        friend class BlockChain;
    public:
        Block(uint64_t height, const std::string& previous_hash, const std::vector<Transaction*>& transactions):
            height_(height),
            previous_hash_(previous_hash),
            transactions_(transactions){}
        Block(): Block(0, "0x00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000", {}){} //TODO: generate previous hash
        Block(Block* parent, const std::vector<Transaction*>& transactions): Block(parent->GetHeight() + 1, parent->GetHash(), transactions){}
        Block(const Messages::Block& raw):
            height_(raw.height()),
            previous_hash_(raw.previous_hash()),
            transactions_(){
            for(auto& it : raw.transactions()) transactions_.push_back(new Transaction(it));
        }
        Block(const Block& other):
            height_(other.height_),
            previous_hash_(other.previous_hash_),
            transactions_(other.transactions_){}
        ~Block(){}

        std::string GetPreviousHash() const{
            return previous_hash_;
        }

        uint64_t GetHeight() const{
            return height_;
        }

        uintptr_t GetNumberOfTransactions() const{
            return transactions_.size();
        }

        Transaction* GetTransactionAt(uintptr_t idx) const{
            if(idx < 0 || idx > GetNumberOfTransactions()) return nullptr;
            return new Transaction(*transactions_[idx]); //TODO: remove allocation?
        }

        Transaction* GetCoinbaseTransaction() const{
            return GetTransactionAt(0);
        }

        bool IsGenesis(){
            return GetHeight() == 0;
        }

        bool Accept(BlockVisitor* vis);
        std::string GetHash();

        std::string GetMerkleRoot(){
            return GetBlockMerkleRoot(this);
        }

        Block& operator=(const Block& other){
            height_ = other.height_;
            previous_hash_ = other.previous_hash_;
            transactions_ = std::vector<Transaction*>(other.transactions_);
            return (*this);
        }

        friend bool operator==(const Block& lhs, const Block& rhs){
            return const_cast<Block&>(lhs).GetHash() == const_cast<Block&>(rhs).GetHash();
        }

        friend bool operator!=(const Block& lhs, const Block& rhs){
            return !operator==(lhs, rhs);
        }

        friend Messages::Block& operator<<(Messages::Block& stream, const Block& block){
            stream.set_height(block.GetHeight());
            stream.set_previous_hash(block.GetPreviousHash());
            for(size_t idx = 0; idx < block.GetNumberOfTransactions(); idx++){
                Messages::Transaction* raw = stream.add_transactions();
                (*raw) << block.transactions_[idx];
            }
            return stream;
        }

        friend Messages::Block& operator<<(Messages::Block& stream, const Block* block){
            stream.set_height(block->GetHeight());
            stream.set_previous_hash(block->GetPreviousHash());
            for(size_t idx = 0; idx < block->GetNumberOfTransactions(); idx++){
                Messages::Transaction* raw = stream.add_transactions();
                (*raw) << block->transactions_[idx];
            }
            return stream;
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
}

#endif //TOKEN_BLOCK_H