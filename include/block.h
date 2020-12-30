#ifndef TOKEN_BLOCK_H
#define TOKEN_BLOCK_H

#include <json/json.h>
#include "hash.h"
#include "bloom.h"
#include "object_tag.h"
#include "utils/buffer.h"
#include "transaction.h"

namespace Token{
    class Block;
    typedef std::shared_ptr<Block> BlockPtr;

    class BlockHeader{
    public:
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
        int64_t height_;
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
        BlockHeader(Timestamp timestamp, int64_t height, const Hash& phash, const Hash& merkle_root, const Hash& hash, const BloomFilter& tx_bloom):
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

        int64_t GetHeight() const{
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

        BlockPtr GetData() const;
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

        static inline int64_t
        GetSize(){
            return sizeof(Timestamp)
                 + sizeof(int64_t)
                 + Hash::GetSize()
                 + Hash::GetSize()
                 + Hash::GetSize();
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
        static const int64_t kMaxTransactionsForBlock = 40000;
        static const int64_t kNumberOfGenesisOutputs = 10000; // TODO: changeme

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
            size += Hash::GetSize(); // previous_hash_
            size += sizeof(int64_t); // num_transactions
            for(auto& it : transactions_)
                size += it->GetBufferSize();
            return size;
        }

        bool Write(Buffer* buff) const{
            buff->PutLong(timestamp_);
            buff->PutLong(height_);
            buff->PutHash(previous_hash_);
            buff->PutLong(GetNumberOfTransactions());
            for(auto& it : transactions_)
                it->Write(buff);
            return true;
        }
    public:
        Block():
            BinaryObject(),
            timestamp_(0),
            height_(0),
            previous_hash_(),
            transactions_(),
            tx_bloom_(){}
        Block(int64_t height, const Hash& phash, const TransactionList& transactions, Timestamp timestamp=GetCurrentTimestamp()):
            BinaryObject(),
            timestamp_(timestamp),
            height_(height),
            previous_hash_(phash),
            transactions_(transactions),
            tx_bloom_(){
            if(!transactions.empty()){
                for(auto& it : transactions)
                    tx_bloom_.Put(it->GetHash());
            }
        }
        Block(const BlockPtr& parent, const TransactionList& transactions, Timestamp timestamp=GetCurrentTimestamp()):
            Block(parent->GetHeight() + 1, parent->GetHash(), transactions, timestamp){}
        Block(const BlockHeader& parent, const TransactionList& transactions, Timestamp timestamp=GetCurrentTimestamp()):
            Block(parent.GetHeight() + 1, parent.GetHash(), transactions, timestamp){}
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

        TransactionPtr& operator[](int64_t idx){
            return transactions_[idx];
        }

        TransactionPtr operator[](int64_t idx) const{
            return transactions_[idx];
        }

        bool IsGenesis(){
            return GetHeight() == 0;
        }

        std::string ToString() const{
            std::stringstream stream;
            stream << "Block(#" << GetHeight() << ", " << GetNumberOfTransactions() << " Transactions)";
            return stream.str();
        }

        Hash GetMerkleRoot() const;
        bool Accept(BlockVisitor* vis) const;
        bool Contains(const Hash& hash) const;

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

        static BlockPtr Genesis();
        static BlockPtr NewInstance(Buffer* buff){
            Timestamp timestamp = buff->GetLong();
            int64_t height = buff->GetLong();
            Hash previous_hash = buff->GetHash();

            int64_t idx;
            int64_t num_transactions = buff->GetLong();
            TransactionList transactions;
            for(idx = 0; idx < num_transactions; idx++)
                transactions.push_back(Transaction::NewInstance(buff));

            return std::make_shared<Block>(height, previous_hash, transactions, timestamp);
        }

        static inline BlockPtr
        NewInstance(const BlockPtr& parent, const TransactionList& txs, const Timestamp& timestamp=GetCurrentTimestamp()){
            return BlockPtr(new Block(parent, txs, timestamp));
        }
    };

    class BlockVisitor{
    protected:
        BlockVisitor() = default;
    public:
        virtual ~BlockVisitor() = default;
        virtual bool VisitStart(){ return true; }
        virtual bool Visit(const TransactionPtr& tx) = 0;
        virtual bool VisitEnd(){ return true; }
    };

    class BlockFileWriter : BinaryFileWriter{
    private:
        TransactionFileWriter tx_writer_;
    public:
        BlockFileWriter(const std::string& filename):
            BinaryFileWriter(filename),
            tx_writer_(this){}
        ~BlockFileWriter() = default;

        bool Write(const BlockPtr& blk){
            WriteObjectTag(ObjectTag::kBlock);

            WriteLong(blk->GetTimestamp()); // timestamp_
            WriteLong(blk->GetHeight()); // height_
            WriteHash(blk->GetPreviousHash()); // previous_hash_
            WriteLong(blk->GetNumberOfTransactions()); // num_transactions_
            for(auto& it : blk->transactions())
                tx_writer_.Write(it); // transactions_[idx]
            return true;
        }
    };

    class BlockFileReader : BinaryFileReader{
    private:
        ObjectTagVerifier tag_verifier_;
        TransactionFileReader tx_reader_;

        inline ObjectTagVerifier&
        GetTagVerifier(){
            return tag_verifier_;
        }

        inline TransactionFileReader&
        GetTransactionReader(){
            return tx_reader_;
        }
    public:
        BlockFileReader(const std::string& filename):
            BinaryFileReader(filename),
            tag_verifier_(this, ObjectTag::kBlock),
            tx_reader_(this){}
        BlockFileReader(BinaryFileReader* parent):
            BinaryFileReader(parent),
            tag_verifier_(this, ObjectTag::kBlock),
            tx_reader_(this){}
        ~BlockFileReader() = default;

        BlockPtr Read(){
            if(!GetTagVerifier().IsValid()){
                LOG(WARNING) << "cannot read block from: " << GetFilename();
                return BlockPtr(nullptr);
            }

            Timestamp timestamp = ReadLong();
            int64_t height = ReadLong();
            Hash phash = ReadHash();

            TransactionList transactions;
            int64_t num_transactions = ReadLong();
            for(int64_t idx = 0; idx < num_transactions; idx++)
                transactions.push_back(GetTransactionReader().Read());
            return std::make_shared<Block>(height, phash, transactions, timestamp);
        }
    };

    class BlockPrinter : Printer{
    public:
        BlockPrinter(const google::LogSeverity& severity=google::INFO, const long& flags=Printer::kFlagNone):
            Printer(severity, flags){}
        BlockPrinter(Printer* parent):
            Printer(parent){}
        ~BlockPrinter() = default;

        bool Print(const BlockPtr& blk) const{
            if(!IsDetailed()){
                LOG_AT_LEVEL(GetSeverity()) << blk->GetHash();
                return true;
            }

            LOG_AT_LEVEL(GetSeverity()) << "Block #" << blk->GetHeight() << ":";
            LOG_AT_LEVEL(GetSeverity()) << "  Timestamp: " << GetTimestampFormattedReadable(blk->GetTimestamp());
            LOG_AT_LEVEL(GetSeverity()) << "  Height: " << blk->GetHeight();
            LOG_AT_LEVEL(GetSeverity()) << "  Previous Hash: " << blk->GetPreviousHash();
            LOG_AT_LEVEL(GetSeverity()) << "  Merkle Root: " << blk->GetMerkleRoot();
            LOG_AT_LEVEL(GetSeverity()) << "  Hash: " << blk->GetHash();
            return true;
        }
    };
}

#endif //TOKEN_BLOCK_H