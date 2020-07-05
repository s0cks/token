#ifndef TOKEN_BLOCK_H
#define TOKEN_BLOCK_H

#include "common.h"
#include "uint256_t.h"
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
        typedef std::vector<Transaction*> TransactionList; //TODO: refactor typing?
    private:
        uint32_t timestamp_;
        uint32_t height_;
        uint256_t previous_hash_;
        std::vector<Transaction*> transactions_;
        std::map<uint256_t, Transaction*> transactions_map_;

        Block(uint32_t timestamp, uint32_t height, const uint256_t& phash, std::vector<Transaction*> txs):
            timestamp_(timestamp),
            height_(height),
            previous_hash_(phash),
            transactions_(txs),
            transactions_map_(){}

        bool Encode(RawType& raw) const;

        friend class BlockChain;
        friend class BlockMessage;
    public:
        ~Block(){}

        BlockHeader GetHeader() const{
            return BlockHeader(timestamp_, height_, previous_hash_, GetMerkleRoot(), GetHash());
        }

        uint256_t GetMerkleRoot() const;

        uint256_t GetPreviousHash() const{
            return previous_hash_;
        }

        uint32_t GetHeight() const{
            return height_;
        }

        uint32_t GetNumberOfTransactions() const{
            return transactions_.size();
        }

        uint32_t GetTimestamp() const{
            return timestamp_;
        }

        Transaction* GetTransaction(uint32_t idx) const{
            if(idx < 0 || idx > transactions_.size()) return nullptr;
            return transactions_[idx];
        }

        Transaction* GetTransaction(const uint256_t& hash) const{
            for(auto& it : transactions_){
                if(it->GetHash() == hash) return it;
            }
            return nullptr;
        }

        std::vector<Transaction*>::iterator begin(){
            return transactions_.begin();
        }

        std::vector<Transaction*>::iterator end(){
            return transactions_.end();
        }

        bool IsGenesis(){
            return GetHeight() == 0;
        }

        bool Finalize();
        bool Contains(const uint256_t& hash) const;
        bool Accept(BlockVisitor* vis) const;
        std::string ToString() const;

        static Block* NewInstance(uint32_t height, const uint256_t& phash, std::vector<Transaction*>& transactions, uint32_t timestamp=GetCurrentTime());
        static Block* NewInstance(const BlockHeader& parent, std::vector<Transaction*>& transactions, uint32_t timestamp=GetCurrentTime());
        static Block* NewInstance(RawType raw);
        static Block* NewInstance(std::fstream& fd);

        static inline Block* NewInstance(const std::string& filename){
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