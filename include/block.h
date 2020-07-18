#ifndef TOKEN_BLOCK_H
#define TOKEN_BLOCK_H

#include "common.h"
#include "uint256_t.h"
#include "transaction.h"
#include "bloom.h"
#include "allocator.h"

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

    class BlockVisitor;
    class Block : public BinaryObject<Proto::BlockChain::Block>{
    public:
        typedef Proto::BlockChain::Block RawType;
    private:
        uint32_t timestamp_;
        uint32_t height_;
        uint256_t previous_hash_;
        size_t transactions_len_;
        Transaction** transactions_;
        BloomFilter tx_bloom_;

        Block(uint32_t timestamp, uint32_t height, const uint256_t& phash, Transaction** txs, uint32_t num_txs):
            timestamp_(timestamp),
            height_(height),
            previous_hash_(phash),
            transactions_len_(num_txs),
            transactions_(nullptr),
            tx_bloom_(){
            transactions_ = (Transaction**)malloc(sizeof(Transaction*)*num_txs);
            memset(transactions_, 0, sizeof(Transaction*)*num_txs);
            memmove(transactions_, txs, sizeof(Transaction*)*num_txs); // should this be moved?
            for(uint32_t idx = 0; idx < num_txs; idx++){
                tx_bloom_.Put(transactions_[idx]->GetSHA256Hash());
            }
        }

        bool Encode(RawType& raw) const;

        friend class BlockChain;
        friend class BlockMessage;
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
            return transactions_len_;
        }

        uint32_t GetTimestamp() const{
            return timestamp_;
        }

        Transaction* GetTransaction(uint32_t idx) const{
            if(idx < 0 || idx > transactions_len_) return nullptr;
            return transactions_[idx];
        }

        bool IsGenesis(){
            return GetHeight() == 0;
        }

        bool Finalize();
        bool Contains(const uint256_t& hash) const;
        bool Accept(BlockVisitor* vis) const;
        std::string ToString() const;

        static Handle<Block> NewInstance(uint32_t height, const uint256_t& phash, Transaction** transactions, uint32_t num_transactions, uint32_t timestamp=GetCurrentTime());
        static Handle<Block> NewInstance(const BlockHeader& parent, Transaction** transactions, uint32_t num_transactions, uint32_t timestamp=GetCurrentTime());
        static Handle<Block> NewInstance(RawType raw);
        static Handle<Block> NewInstance(std::fstream& fd);

        static inline Handle<Block> NewInstance(const std::string& filename){
            std::fstream fd(filename, std::ios::in|std::ios::binary);
            return NewInstance(fd);
        }
    };

    class BlockVisitor{
    public:
        BlockVisitor(){}
        virtual ~BlockVisitor(){}

        virtual bool VisitStart(){ return true; }
        virtual bool Visit(Transaction* tx) = 0;
        virtual bool VisitEnd(){ return true; }
    };

    class BlockPoolVisitor;
    class BlockPool{
    public:
        enum State{
            kUninitialized,
            kInitializing,
            kInitialized,
        };
    private:
        BlockPool() = delete;

        static void SetState(State state);
    public:
        ~BlockPool() = delete;

        static State GetState();
        static void Initialize();
        static void RemoveBlock(const uint256_t& hash);
        static void PutBlock(Block* block);
        static bool HasBlock(const uint256_t& hash);
        static bool Accept(BlockPoolVisitor* vis);
        static bool GetBlocks(std::vector<uint256_t>& blocks);
        static Block* GetBlock(const uint256_t& hash);

        static inline bool
        IsUninitialized(){
            return GetState() == kUninitialized;
        }

        static inline bool
        IsInitializing(){
            return GetState() == kInitializing;
        }

        static inline bool
        IsInitialized(){
            return GetState() == kInitialized;
        }
    };
}

#endif //TOKEN_BLOCK_H