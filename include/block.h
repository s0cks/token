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
        typedef Proto::BlockChain::BlockHeader RawType;
    private:
        uint32_t timestamp_;
        uint32_t height_;
        uint256_t previous_hash_;
        uint256_t merkle_root_;
        uint256_t hash_;
    public:
        BlockHeader():
            timestamp_(0),
            height_(0),
            previous_hash_(),
            merkle_root_(),
            hash_(){}
        BlockHeader(uint32_t timestamp, uint32_t height, const uint256_t& phash, const uint256_t& merkle_root, const uint256_t& hash):
            timestamp_(timestamp),
            height_(height),
            previous_hash_(phash),
            merkle_root_(merkle_root),
            hash_(hash){}
        BlockHeader(Block* blk);
        BlockHeader(const RawType& raw): BlockHeader(raw.timestamp(), raw.height(), HashFromHexString(raw.previous_hash()), HashFromHexString(raw.merkle_root()), HashFromHexString(raw.hash())){}
        ~BlockHeader(){}

        uint32_t GetTimestamp() const{
            return timestamp_;
        }

        uint32_t GetHeight() const{
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

    typedef Proto::BlockChain::Block RawBlock;

    class BlockVisitor;
    class Block : public BinaryObject<RawBlock>{
        //TODO:
        // - validation logic
        // - refactor to use raw array
        friend class BlockChain;
        friend class BlockMessage;
    private:
        uint32_t timestamp_;
        uint32_t height_;
        uint256_t previous_hash_;
        Transaction** transactions_;
        size_t num_transactions_;
        BloomFilter tx_bloom_;

        Block(uint32_t timestamp, uint32_t height, const uint256_t& phash, const Handle<Array<Transaction>>& txs):
            BinaryObject(),
            timestamp_(timestamp),
            height_(height),
            previous_hash_(phash),
            transactions_(nullptr),
            num_transactions_(txs->Length()),
            tx_bloom_(){
            transactions_ = (Transaction**)malloc(sizeof(Transaction*)*txs->Length());
            memset(transactions_, 0, sizeof(Transaction*)*txs->Length());
            for(size_t idx = 0; idx < txs->Length(); idx++){
                WriteBarrier(&transactions_[idx], txs->Get(idx));
                tx_bloom_.Put(txs->Get(idx)->GetSHA256Hash());
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
            return BlockHeader(timestamp_, height_, previous_hash_, GetMerkleRoot(), GetSHA256Hash());
        }

        uint256_t GetMerkleRoot() const;

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

        bool Accept(BlockVisitor* vis) const;
        bool Contains(const uint256_t& hash) const;
        bool WriteToMessage(RawBlock& raw) const;
        std::string ToString() const;

        static Handle<Block> Genesis(); // genesis
        static Handle<Block> NewInstance(uint32_t height, const uint256_t& phash, const Handle<Array<Transaction>>& txs, uint32_t timestamp=GetCurrentTime());
        static Handle<Block> NewInstance(const BlockHeader& parent, const Handle<Array<Transaction>>& txs, uint32_t timestamp=GetCurrentTime());
        static Handle<Block> NewInstance(const RawBlock& raw);
        static Handle<Block> NewInstance(std::fstream& fd);

        static inline Handle<Block> NewInstance(const std::string& filename){
            std::fstream fd(filename, std::ios::in|std::ios::binary);
            return NewInstance(fd);
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