#ifndef TOKEN_BLOCK_H
#define TOKEN_BLOCK_H

#include "common.h"
#include "uint256_t.h"
#include "bloom.h"

#include "allocator.h"
#include "array.h"
#include "transaction.h"

namespace Token{
    class Block;
    class BlockHeader{
    public:
    private:
        uint64_t timestamp_;
        uint64_t height_;
        uint256_t previous_hash_;
        uint256_t merkle_root_;
        uint256_t hash_;
        BloomFilter bloom_;
    public:
        BlockHeader():
            timestamp_(0),
            height_(0),
            previous_hash_(uint256_t()),
            merkle_root_(), // fill w/ genesis's merkle root
            hash_(){} //TODO: fill w/ genesis's hash
        BlockHeader(uint32_t timestamp, uint32_t height, const uint256_t& phash, const uint256_t& merkle_root, const uint256_t& hash, const BloomFilter& tx_bloom):
            timestamp_(timestamp),
            height_(height),
            previous_hash_(phash),
            merkle_root_(merkle_root),
            hash_(hash),
            bloom_(tx_bloom){}
        BlockHeader(Block* blk);
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

        uint256_t GetMerkleRoot() const{
            return merkle_root_;
        }

        uint256_t GetHash() const{
            return hash_;
        }

        Block* GetData() const;

        bool Contains(const uint256_t& hash) const{
            return bloom_.Contains(hash);
        }

        BlockHeader& operator=(const BlockHeader& other){
            timestamp_ = other.timestamp_;
            height_ = other.height_;
            previous_hash_ = other.previous_hash_;
            merkle_root_ = other.merkle_root_;
            hash_ = other.hash_;
            return (*this);
        }

        friend bool operator==(const BlockHeader& a, const BlockHeader& b){
            return a.GetHash() == b.GetHash();
        }

        friend bool operator!=(const BlockHeader& a, const BlockHeader& b){
            return a.GetHash() != b.GetHash();
        }

        friend bool operator<(const BlockHeader& a, const BlockHeader& b){
            return a.GetHeight() < b.GetHeight();
        }

        friend std::ostream& operator<<(std::ostream& stream, const BlockHeader& header){
            stream << "#" << header.GetHeight() << "(" << header.GetHash() << ")";
            return stream;
        }
    };

    class BlockVisitor;
    class Block : public Object{
        //TODO:
        // - validation logic
        friend class BlockHeader;
        friend class BlockChain;
        friend class BlockMessage;
    public:
        static const size_t kMaxTransactionsForBlock = 20;
        static const uint32_t kNumberOfGenesisOutputs = 40000;
    private:
        uint32_t timestamp_;
        uint32_t height_;
        uint256_t previous_hash_;
        Transaction** transactions_;
        size_t num_transactions_;
        BloomFilter tx_bloom_; // transient

        Block(uint32_t timestamp, uint32_t height, const uint256_t& phash, Transaction** txs, size_t num_txs):
            Object(),
            timestamp_(timestamp),
            height_(height),
            previous_hash_(phash),
            transactions_(nullptr),
            num_transactions_(num_txs),
            tx_bloom_(){
            transactions_ = (Transaction**)malloc(sizeof(Transaction*)*num_txs);
            memset(transactions_, 0, sizeof(Transaction*)*num_txs);
            for(size_t idx = 0; idx < num_txs; idx++){
                WriteBarrier(&transactions_[idx], txs[idx]);
                tx_bloom_.Put(txs[idx]->GetHash());
            }
        }
    protected:
        virtual void Accept(WeakReferenceVisitor* vis){
            for(size_t idx = 0; idx < num_transactions_; idx++){
                vis->Visit(&transactions_[idx]);
            }
        }
    public:
        ~Block() = default;

        BlockHeader GetHeader() const{
            return BlockHeader(timestamp_, height_, previous_hash_, GetMerkleRoot(), GetHash(), tx_bloom_);
        }

        uint256_t GetPreviousHash() const{
            return previous_hash_;
        }

        uint32_t GetHeight() const{
            return height_;
        }

        uint32_t GetNumberOfTransactions() const{
            return num_transactions_;
        }

        uint32_t GetTimestamp() const{
            return timestamp_;
        }

        Handle<Transaction> GetTransaction(uint32_t idx) const{
            if(idx < 0 || idx > GetNumberOfTransactions()) return Handle<Transaction>();
            return transactions_[idx];
        }

        bool IsGenesis(){
            return GetHeight() == 0;
        }

        uint256_t GetMerkleRoot() const;
        size_t GetBufferSize() const;
        bool Encode(ByteBuffer* bytes) const;
        bool Accept(BlockVisitor* vis) const;
        bool Contains(const uint256_t& hash) const;
        std::string ToString() const;

        static Handle<Block> Genesis(); // genesis
        static Handle<Block> NewInstance(std::fstream& fd, size_t size);
        static Handle<Block> NewInstance(ByteBuffer* bytes);

        static Handle<Block> NewInstance(uint32_t height, const uint256_t& phash, Transaction** txs, size_t num_txs, uint64_t timestamp=GetCurrentTime()){
            return new Block(timestamp, height, phash, txs, num_txs);
        }

        static Handle<Block> NewInstance(const BlockHeader& previous, Transaction** txs, size_t num_txs, uint64_t timestamp=GetCurrentTimestamp()){
            return new Block(timestamp, previous.GetHeight() + 1, previous.GetHash(), txs, num_txs);
        }

        static inline Handle<Block> NewInstance(const std::string& filename){
            std::fstream fd(filename, std::ios::in|std::ios::binary);
            return NewInstance(fd, GetFilesize(filename));//TODO: refactor?
        }
    };

    class BlockVisitor{
    protected:
        BlockVisitor() = default;
    public:
        virtual ~BlockVisitor() = default;
        virtual bool VisitStart(){ return true; }
        virtual bool Visit(const Handle<Transaction>& tx) = 0;
        virtual bool VisitEnd(){ return true; }
    };
}

#endif //TOKEN_BLOCK_H