#ifndef TOKEN_BLOCK_H
#define TOKEN_BLOCK_H

#include "common.h"
#include "uint256_t.h"
#include "bloom.h"

#include "allocator.h"
#include "array.h"
#include "node.h"
#include "transaction.h"

namespace Token{
    class Block;
    class BlockHeader{
    public:
        static const size_t kSize = sizeof(Timestamp) +
                                    sizeof(intptr_t) +
                                    uint256_t::kSize +
                                    uint256_t::kSize +
                                    uint256_t::kSize;
    private:
        Timestamp timestamp_;
        intptr_t height_;
        uint256_t previous_hash_;
        uint256_t merkle_root_;
        uint256_t hash_;
        BloomFilter bloom_;
    public:
        BlockHeader():
            timestamp_(0),
            height_(0),
            previous_hash_(uint256_t::Null()),
            merkle_root_(uint256_t::Null()), // fill w/ genesis's merkle root
            hash_(uint256_t::Null()), //TODO: fill w/ genesis's hash
            bloom_(){}
        BlockHeader(const BlockHeader& blk):
            timestamp_(blk.timestamp_),
            height_(blk.height_),
            previous_hash_(blk.previous_hash_),
            merkle_root_(blk.merkle_root_),
            hash_(blk.hash_),
            bloom_(blk.bloom_){}
        BlockHeader(Timestamp timestamp, intptr_t height, const uint256_t& phash, const uint256_t& merkle_root, const uint256_t& hash, const BloomFilter& tx_bloom):
            timestamp_(timestamp),
            height_(height),
            previous_hash_(phash),
            merkle_root_(merkle_root),
            hash_(hash),
            bloom_(tx_bloom){}
        BlockHeader(Block* blk);
        BlockHeader(ByteBuffer* bytes);
        ~BlockHeader(){}

        Timestamp GetTimestamp() const{
            return timestamp_;
        }

        intptr_t GetHeight() const{
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

        bool Contains(const uint256_t& hash) const{
            return bloom_.Contains(hash);
        }

        Handle<Block> GetData() const;
        bool Encode(ByteBuffer* bytes) const;

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
    class Block : public BinaryObject{
        //TODO:
        // - validation logic
        friend class BlockHeader;
        friend class BlockChain;
        friend class BlockMessage;
    public:
        static const size_t kMaxTransactionsForBlock = 20;
        static const uint32_t kNumberOfGenesisOutputs = 32; // TODO: changeme

        struct TimestampComparator{
            bool operator()(Block* a, Block* b){
                return a->GetTimestamp() < b->GetTimestamp();
            }
        };

        struct HeightComparator{
            bool operator()(Block* a, Block* b){
                if(!a) return -1;
                if(!b) return 1;
                return a->GetHeight() < b->GetHeight();
            }
        };
    private:
        Timestamp timestamp_;
        intptr_t height_;
        uint256_t previous_hash_;
        Transaction** transactions_;
        intptr_t num_transactions_;
        BloomFilter tx_bloom_; // transient

        Block(Timestamp timestamp, uint32_t height, const uint256_t& phash, Transaction** txs, intptr_t num_txs):
            BinaryObject(),
            timestamp_(timestamp),
            height_(height),
            previous_hash_(phash),
            transactions_(nullptr),
            num_transactions_(num_txs),
            tx_bloom_(){
            SetType(Type::kBlockType);

            if(num_txs > 0){
                transactions_ = (Transaction**)malloc(sizeof(Transaction*)*num_txs);
                memset(transactions_, 0, sizeof(Transaction*)*num_txs);
                for(intptr_t idx = 0; idx < num_txs; idx++){
                    WriteBarrier(&transactions_[idx], txs[idx]);
                }
            }

            //TODO: tx_bloom_.Put(txs[idx]->GetHash());
        }
    protected:
        bool Accept(WeakObjectPointerVisitor* vis){
            return vis->Visit(&transactions_);
        }

        bool Write(ByteBuffer* bytes) const{
            bytes->PutLong(timestamp_);
            bytes->PutLong(height_);
            bytes->PutHash(previous_hash_);
            bytes->PutLong(num_transactions_);
            for(intptr_t idx = 0;
                idx < num_transactions_;
                idx++){
                transactions_[idx]->Write(bytes);
            }
            return true;
        }
    public:
        ~Block() = default;

        BlockHeader GetHeader() const{
            return BlockHeader(timestamp_, height_, previous_hash_, GetMerkleRoot(), GetHash(), tx_bloom_);
        }

        uint256_t GetPreviousHash() const{
            return previous_hash_;
        }

        intptr_t GetHeight() const{
            return height_;
        }

        intptr_t GetNumberOfTransactions() const{
            return num_transactions_;
        }

        Timestamp GetTimestamp() const{
            return timestamp_;
        }

        Handle<Transaction> GetTransaction(intptr_t idx) const{
            if(idx < 0 || idx > GetNumberOfTransactions())
                return nullptr;
            return transactions_[idx];
        }

        bool IsGenesis(){
            return GetHeight() == 0;
        }

        uint256_t GetMerkleRoot() const;
        bool Accept(BlockVisitor* vis) const;
        bool Contains(const uint256_t& hash) const;
        std::string ToString() const;

        static Handle<Block> Genesis(); // genesis
        static Handle<Block> NewInstance(std::fstream& fd, size_t size);
        static Handle<Block> NewInstance(ByteBuffer* bytes);

        static Handle<Block> NewInstance(intptr_t height, const uint256_t& phash, Transaction** txs, size_t num_txs, Timestamp timestamp=GetCurrentTimestamp()){
            return new Block(timestamp, height, phash, txs, num_txs);
        }

        static Handle<Block> NewInstance(const BlockHeader& previous, Transaction** txs, size_t num_txs, Timestamp timestamp=GetCurrentTimestamp()){
            return new Block(timestamp, previous.GetHeight() + 1, previous.GetHash(), txs, num_txs);
        }

        static inline Handle<Block> NewInstance(const std::string& filename){
            std::fstream fd(filename, std::ios::in|std::ios::binary);
            LOG(INFO) << "reading " << GetFilesize(filename) << " bytes from file";
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

    class BlockPrinter : public BlockVisitor{
    private:
        bool detailed_;
    public:
        BlockPrinter(bool is_detailed=false):
            BlockVisitor(),
            detailed_(is_detailed){}
        ~BlockPrinter() = default;

        bool IsDetailed() const{
            return detailed_;
        }

        bool Visit(const Handle<Transaction>& tx){
            LOG(INFO) << "#" << tx->GetIndex() << ". " << tx->GetHash();
            return true;
        }
    };
}

#endif //TOKEN_BLOCK_H