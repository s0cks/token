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
        static const int64_t kMaxTransactionsForBlock = 20;
        static const int64_t kNumberOfGenesisOutputs = 32; // TODO: changeme

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
        int64_t height_;
        Hash previous_hash_;
        TransactionList transactions_;
        BloomFilter tx_bloom_; // transient
    protected:
        int64_t GetBufferSize() const{
            int64_t size = 0;
            size += sizeof(Timestamp); // timestamp_
            size += sizeof(int64_t); // height_
            size += Hash::kSize; // previous_hash_
            size += sizeof(int64_t); // num_transactions
            for(auto& it : transactions_)
                size += it.GetBufferSize();
            return size;
        }

        bool Encode(Buffer* buff) const{
            buff->PutLong(timestamp_);
            buff->PutLong(height_);
            buff->PutHash(previous_hash_);
            buff->PutLong(GetNumberOfTransactions());
            for(auto& it : transactions_)
                it.Encode(buff);
            return true;
        }
    public:
        Block():
            BinaryObject(Type::kBlockType),
            timestamp_(0),
            height_(0),
            previous_hash_(),
            transactions_(),
            tx_bloom_(){}
        Block(int64_t height, const Hash& phash, const TransactionList& transactions, Timestamp timestamp=GetCurrentTimestamp()):
            BinaryObject(Type::kBlockType),
            timestamp_(timestamp),
            height_(height),
            previous_hash_(phash),
            transactions_(transactions),
            tx_bloom_(){
            if(!transactions.empty()){
                for(auto& it : transactions)
                    tx_bloom_.Put(it.GetHash());
            }
        }
        Block(const Block& parent, const TransactionList& transactions, Timestamp timestamp=GetCurrentTimestamp()):
            Block(parent.GetHeight() + 1, parent.GetHash(), transactions, timestamp){}
        Block(Block* parent, const TransactionList& transactions, Timestamp timestamp=GetCurrentTimestamp()):
            Block(parent->GetHeight() + 1, parent->GetHash(), transactions, timestamp){}
        Block(const BlockHeader& parent, const TransactionList& transactions, Timestamp timestamp=GetCurrentTimestamp()):
            Block(parent.GetHeight() + 1, parent.GetHash(), transactions, timestamp){}
        Block(Buffer* buff):
            BinaryObject(Type::kBlockType),
            timestamp_(buff->GetLong()),
            height_(buff->GetLong()),
            previous_hash_(buff->GetHash()),
            transactions_(),
            tx_bloom_(){
            int64_t idx;
            int64_t num_transactions = buff->GetLong();
            for(idx = 0;
                idx < num_transactions;
                idx++){
                Transaction tx(buff);
                transactions_.push_back(tx);
                tx_bloom_.Put(tx.GetHash());
            }
        }
        Block(const Block& other):
            BinaryObject(Type::kBlockType),
            timestamp_(other.timestamp_),
            height_(other.height_),
            previous_hash_(other.previous_hash_),
            transactions_(other.transactions_),
            tx_bloom_(other.tx_bloom_){}
        ~Block() = default;

        BlockHeader GetHeader() const{
            return BlockHeader(timestamp_, height_, previous_hash_, GetMerkleRoot(), GetHash(), tx_bloom_);
        }

        Timestamp GetTimestamp() const{
            return timestamp_;
        }

        int64_t GetHeight() const{
            return height_;
        }

        Hash GetPreviousHash() const{
            return previous_hash_;
        }

        int64_t GetNumberOfTransactions() const{
            return transactions_.size();
        }

        TransactionList& transactions(){
            return transactions_;
        }

        TransactionList transactions() const{
            return transactions_;
        }

        TransactionList::iterator transactions_begin(){
            return transactions_.begin();
        }

        TransactionList::const_iterator transactions_begin() const{
            return transactions_.begin();
        }

        TransactionList::iterator transactions_end(){
            return transactions_.end();
        }

        TransactionList::const_iterator transactions_end() const{
            return transactions_.end();
        }

        Transaction& operator[](int64_t idx){
            return transactions_[idx];
        }

        Transaction operator[](int64_t idx) const{
            return transactions_[idx];
        }

        bool IsGenesis(){
            return GetHeight() == 0;
        }

        Buffer* ToBuffer() const{
            Buffer buff(GetBufferSize());
            if(!Encode(&buff)){
                LOG(WARNING) << "couldn't encode block to buffer";
                return nullptr;
            }
            return new Buffer(buff);
        }

        std::string ToString() const{
            std::stringstream stream;
            stream << "Block(#" << GetHeight() << ", " << GetNumberOfTransactions() << " Transactions)";
            return stream.str();
        }

        Hash GetMerkleRoot() const;
        bool Accept(BlockVisitor* vis) const;
        bool Contains(const Hash& hash) const;
        bool ToJson(Json::Value& value) const;

        void operator=(const Block& other){
            timestamp_ = other.timestamp_;
            height_ = other.height_;
            previous_hash_ = other.previous_hash_;
            transactions_ = other.transactions_;
        }

        friend bool operator==(const Block& a, const Block& b){
            return a.timestamp_ == b.timestamp_
                && a.height_ == b.height_
                && a.GetHash() == b.GetHash();
        }

        friend bool operator!=(const Block& a, const Block& b){
            return !operator==(a, b);
        }

        friend bool operator<(const Block& a, const Block& b){
            return a.height_ < b.height_;
        }

        friend std::ostream& operator<<(std::ostream& stream, const Block& blk){
            stream << blk.ToString();
            return stream;
        }

        static Block Genesis();

        static inline Block*
        NewInstance(const std::string& filename){
            std::fstream fd(filename, std::ios::binary|std::ios::in);
            int64_t total_size = GetFilesize(filename);
            Buffer buffer(total_size);
            if(!buffer.ReadBytesFrom(fd, total_size)){
                LOG(WARNING) << "couldn't read block from file: " << filename;
                return nullptr;
            }
            return new Block(&buffer);
        }
    };

    class BlockVisitor{
    protected:
        BlockVisitor() = default;
    public:
        virtual ~BlockVisitor() = default;
        virtual bool VisitStart() const{ return true; }
        virtual bool Visit(const Transaction& tx) = 0;
        virtual bool VisitEnd() const{ return true; }
    };

    class BlockPrinter : public BlockVisitor{
    private:
        const Block& block_;

        const Block&
        block() const{
            return block_;
        }

        BlockPrinter(const Block& blk):
            block_(blk){}
    public:
        ~BlockPrinter() = default;

        bool VisitStart() const{
            LOG(INFO) << "Block #" << block().GetHeight();
            LOG(INFO) << "  - Created: " << GetTimestampFormattedReadable(block().GetTimestamp());
            LOG(INFO) << "  - Previous Hash: " << block().GetPreviousHash();
            LOG(INFO) << "  - Merkle Root: " << block().GetMerkleRoot();
            LOG(INFO) << "  - Hash: " << block().GetHash();
            LOG(INFO) << "Transactions (" << block().GetNumberOfTransactions() << "):";
            return true;
        }

        bool Visit(const Transaction& tx){
            LOG(INFO) << "  - #" << tx.GetIndex() << ": " << tx.GetHash();
            return true;
        }

        static inline bool
        Print(const Block& blk){
            BlockPrinter printer(blk);
            return blk.Accept(&printer);
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