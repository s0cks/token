#ifndef TOKEN_BLOCK_H
#define TOKEN_BLOCK_H

#include <json/json.h>
#include "hash.h"
#include "bloom.h"
#include "buffer.h"
#include "transaction.h"

namespace Token{
    class Block;
    class BlockHeader{
    public:
        static const size_t kSize = sizeof(Timestamp) +
                                    sizeof(intptr_t) +
                                    Hash::kSize +
                                    Hash::kSize +
                                    Hash::kSize;

        struct TimestampComparator{
            bool operator()(const BlockHeader& a, const BlockHeader& b){
                return a.timestamp_ < b.timestamp_;
            }
        };

        struct HeightComparator{
            bool operator()(const BlockHeader& a, const BlockHeader& b){
                return a.height_ < b.height_;
            }
        };
    private:
        Timestamp timestamp_;
        intptr_t height_;
        Hash previous_hash_;
        Hash merkle_root_;
        Hash hash_;
        BloomFilter bloom_;
    public:
        BlockHeader():
            timestamp_(0),
            height_(0),
            previous_hash_(),
            merkle_root_(), // fill w/ genesis's merkle root
            hash_(), //TODO: fill w/ genesis's Hash
            bloom_(){}
        BlockHeader(const BlockHeader& blk):
            timestamp_(blk.timestamp_),
            height_(blk.height_),
            previous_hash_(blk.previous_hash_),
            merkle_root_(blk.merkle_root_),
            hash_(blk.hash_),
            bloom_(blk.bloom_){}
        BlockHeader(Timestamp timestamp, intptr_t height, const Hash& phash, const Hash& merkle_root, const Hash& hash, const BloomFilter& tx_bloom):
            timestamp_(timestamp),
            height_(height),
            previous_hash_(phash),
            merkle_root_(merkle_root),
            hash_(hash),
            bloom_(tx_bloom){}
        BlockHeader(Block* blk);
        BlockHeader(Buffer* buff);
        ~BlockHeader(){}

        Timestamp GetTimestamp() const{
            return timestamp_;
        }

        intptr_t GetHeight() const{
            return height_;
        }

        Hash GetPreviousHash() const{
            return previous_hash_;
        }

        Hash GetMerkleRoot() const{
            return merkle_root_;
        }

        Hash GetHash() const{
            return hash_;
        }

        bool Contains(const Hash& hash) const{
            return bloom_.Contains(hash);
        }

        Block* GetData() const;
        bool Write(Buffer* buff) const;

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
        Hash previous_hash_;
        intptr_t num_transactions_;
        Transaction** transactions_;
        BloomFilter tx_bloom_; // transient

        Block(Timestamp timestamp, uint32_t height, const Hash& phash, Transaction** txs, intptr_t num_txs):
            BinaryObject(Type::kBlockType),
            timestamp_(timestamp),
            height_(height),
            previous_hash_(phash),
            num_transactions_(num_txs),
            transactions_(nullptr),
            tx_bloom_(){

            if(num_txs > 0){
                transactions_ = (Transaction**)malloc(sizeof(Transaction*)*num_txs);
                memset(transactions_, 0, sizeof(Transaction*)*num_txs);
                for(intptr_t idx = 0; idx < num_txs; idx++){
                    transactions_[idx] = txs[idx];
                    tx_bloom_.Put(txs[idx]->GetHash());
                }
            }

            //TODO: tx_bloom_.Put(txs[idx]->GetHash());
        }
    protected:
        intptr_t GetBufferSize() const{
            intptr_t size = 0;
            size += sizeof(Timestamp); // timestamp_
            size += sizeof(intptr_t); // height_
            size += Hash::kSize; // previous_hash_
            size += sizeof(intptr_t); // num_transactions
            for(intptr_t idx = 0; idx < num_transactions_; idx++)
                size += transactions_[idx]->GetBufferSize(); // transactions_[idx]
            return size;
        }

        bool Write(Buffer* buff) const;
    public:
        ~Block() = default;

        BlockHeader GetHeader() const{
            return BlockHeader(timestamp_, height_, previous_hash_, GetMerkleRoot(), GetHash(), tx_bloom_);
        }

        Hash GetPreviousHash() const{
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

        Transaction* GetTransaction(int64_t idx) const{
            if(idx < 0 || idx > GetNumberOfTransactions())
                return nullptr;
            return transactions_[idx];
        }

        bool IsGenesis(){
            return GetHeight() == 0;
        }

        Buffer* ToBuffer() const{
            Buffer* buff = new Buffer(GetBufferSize());
            if(!Write(buff)){
                LOG(WARNING) << "couldn't encode block to buffer";
                delete buff;
                return nullptr;
            }
            return buff;
        }

        Hash GetMerkleRoot() const;
        bool Accept(BlockVisitor* vis) const;
        bool Contains(const Hash& hash) const;
        bool Equals(Block* block) const;
        bool Compare(Block* block) const;
        bool ToJson(Json::Value& value) const;
        std::string ToString() const;

        static Block* Genesis();
        static Block* NewInstance(std::fstream& fd, int64_t nbytes);
        static Block* NewInstance(Buffer* buff);

        static Block* NewInstance(intptr_t height, const Hash& phash, Transaction** txs, size_t num_txs, Timestamp timestamp=GetCurrentTimestamp()){
            return new Block(timestamp, height, phash, txs, num_txs);
        }

        static Block* NewInstance(const BlockHeader& previous, Transaction** txs, size_t num_txs, Timestamp timestamp=GetCurrentTimestamp()){
            return new Block(timestamp, previous.GetHeight() + 1, previous.GetHash(), txs, num_txs);
        }

        static Block* NewInstance(const std::string& filename){
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
        virtual bool Visit(Transaction* tx) = 0; //TODO: convert to const
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

        bool Visit(Transaction* tx){
            LOG(INFO) << "#" << tx->GetIndex() << ". " << tx->GetHash();
            return true;
        }
    };

#define FOR_EACH_BLOCK_POOL_STATE(V) \
    V(Uninitialized)                 \
    V(Initializing)                  \
    V(Initialized)

#define FOR_EACH_BLOCK_POOL_STATUS(V) \
    V(Ok)                             \
    V(Warning)                        \
    V(Error)

    class BlockPoolVisitor;
    class BlockPool{
    public:
        enum State{
#define DEFINE_STATE(Name) k##Name,
            FOR_EACH_BLOCK_POOL_STATE(DEFINE_STATE)
#undef DEFINE_STATE
        };

        friend std::ostream& operator<<(std::ostream& stream, const State& state){
            switch(state){
#define DEFINE_TOSTRING(Name) \
                case State::k##Name: \
                    stream << #Name; \
                    return stream;
                FOR_EACH_BLOCK_POOL_STATE(DEFINE_TOSTRING)
#undef DEFINE_TOSTRING
            }
        }

        enum Status{
#define DEFINE_STATUS(Name) k##Name,
            FOR_EACH_BLOCK_POOL_STATUS(DEFINE_STATUS)
#undef DEFINE_STATUS
        };

        friend std::ostream& operator<<(std::ostream& stream, const Status& status){
            switch(status){
#define DEFINE_TOSTRING(Name) \
                case Status::k##Name: \
                    stream << #Name; \
                    return stream;
                FOR_EACH_BLOCK_POOL_STATUS(DEFINE_TOSTRING)
#undef DEFINE_TOSTRING
            }
        }
    private:
        BlockPool() = delete;

        static void SetState(const State& state);
        static void SetStatus(const Status& status);
    public:
        ~BlockPool() = delete;

        static int64_t GetNumberOfBlocksInPool();
        static State GetState();
        static Status GetStatus();
        static std::string GetStatusMessage();
        static bool Initialize();
        static bool Print();
        static bool Accept(BlockPoolVisitor* vis);
        static bool RemoveBlock(const Hash& hash);
        static bool PutBlock(const Hash& hash, Block* block);
        static bool HasBlock(const Hash& hash);
        static bool GetBlocks(std::vector<Hash>& blocks);
        static Block* GetBlock(const Hash& hash);
        static void WaitForBlock(const Hash& hash);

#define DEFINE_STATE_CHECK(Name) \
        static inline bool Is##Name(){ return GetState() == State::k##Name; }
        FOR_EACH_BLOCK_POOL_STATE(DEFINE_STATE_CHECK)
#undef DEFINE_STATE_CHECK

#define DEFINE_STATUS_CHECK(Name) \
        static inline bool Is##Name(){ return GetStatus() == Status::k##Name; }
        FOR_EACH_BLOCK_POOL_STATUS(DEFINE_STATUS_CHECK)
#undef DEFINE_STATUS_CHECK
    };

    class BlockPoolVisitor{
    protected:
        BlockPoolVisitor() = default;
    public:
        virtual ~BlockPoolVisitor() = default;
        virtual bool VisitStart(){ return true; }
        virtual bool Visit(Block* block) = 0;
        virtual bool VisitEnd(){ return true; }
    };
}

#endif //TOKEN_BLOCK_H