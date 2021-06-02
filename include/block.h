#ifndef TOKEN_BLOCK_H
#define TOKEN_BLOCK_H

#include "hash.h"
#include "bloom.h"
#include "transaction.h"
#include "block_header.h"
#include "buffer.h"

namespace token{
  class BlockVisitor;
  class Block : public BinaryObject, public std::enable_shared_from_this<Block>{
    //TODO:
    // - validation logic
    friend class BlockHeader;
    friend class BlockChain;
    friend class BlockMessage;
   public:
#ifdef TOKEN_DEBUG
    static const int64_t kMaxBlockSize = 128 * token::internal::kMegabytes;
    static const int64_t kNumberOfGenesisOutputs = 10000;
#else
    static const int64_t kMaxBlockSize = 1 * token::internal::kGigabytes;
    static const int64_t kNumberOfGenesisOutputs = 10000;
#endif//TOKEN_DEBUG

   private:
    Timestamp timestamp_;
    Version version_;
    int64_t height_;
    Hash previous_hash_;
    IndexedTransactionSet transactions_;
    BloomFilter tx_bloom_; // transient
   public:
    Block():
      BinaryObject(),
      timestamp_(Clock::now()),
      version_(TOKEN_MAJOR_VERSION, TOKEN_MINOR_VERSION, TOKEN_REVISION_VERSION),
      height_(0),
      previous_hash_(),
      transactions_(),
      tx_bloom_(){}
    Block(int64_t height,
      const Version& version,
      const Hash& phash,
      const IndexedTransactionSet& transactions,
      Timestamp timestamp = Clock::now()):
      BinaryObject(),
      timestamp_(timestamp),
      version_(version),
      height_(height),
      previous_hash_(phash),
      transactions_(transactions),
      tx_bloom_(){
      if(!transactions.empty()){
        for(auto& it : transactions)
          tx_bloom_.Put(it->GetHash());
      }
    }
    Block(const BlockPtr& parent, const IndexedTransactionSet& transactions, Timestamp timestamp = Clock::now()):
      Block(parent->GetHeight() + 1, Version(TOKEN_MAJOR_VERSION, TOKEN_MINOR_VERSION, TOKEN_REVISION_VERSION), parent->GetHash(), transactions, timestamp){}
    ~Block() override = default;

    Type GetType() const override{
      return Type::kBlock;
    }

    Timestamp GetTimestamp() const{
      return timestamp_;
    }

    Version GetVersion() const{
      return version_;
    }

    int64_t GetHeight() const{
      return height_;
    }

    Hash GetPreviousHash() const{
      return previous_hash_;
    }

    BlockHeader GetHeader() const{
      return BlockHeader(
        timestamp_,
        version_,
        height_,
        previous_hash_,
        GetMerkleRoot(),
        GetHash()
      );
    }

    BloomFilter& GetBloomFilter(){
      return tx_bloom_;
    }

    BloomFilter GetBloomFilter() const{
      return tx_bloom_;
    }

    int64_t GetNumberOfTransactions() const{
      return transactions_.size();
    }

    IndexedTransactionSet& transactions(){
      return transactions_;
    }

    IndexedTransactionSet transactions() const{
      return transactions_;
    }

    IndexedTransactionSet::iterator transactions_begin(){
      return transactions_.begin();
    }

    IndexedTransactionSet::const_iterator transactions_begin() const{
      return transactions_.begin();
    }

    IndexedTransactionSet::iterator transactions_end(){
      return transactions_.end();
    }

    IndexedTransactionSet::const_iterator transactions_end() const{
      return transactions_.end();
    }

    bool IsGenesis() const{
      return GetHeight() == 0;
    }

    std::string ToString() const override{
      std::stringstream stream;
      stream << "Block(#" << GetHeight() << ", " << GetNumberOfTransactions() << " Transactions)";
      return stream.str();
    }

    int64_t GetTransactionDataBufferSize() const{
      int64_t size = 0;
      size += sizeof(int64_t); // num_transactions
      for(auto& it : transactions_)
        size += it->GetBufferSize();
      return size;
    }

    int64_t GetBufferSize() const override{
      int64_t size = 0;
      size += sizeof(uint64_t); // timestamp_
      size += sizeof(RawVersion); // version_
      size += sizeof(int64_t); // height_
      size += Hash::GetSize(); // previous_hash_
      size += GetTransactionDataBufferSize();
      return size;
    }

    bool Write(const BufferPtr& buff) const override{
      return buff->PutTimestamp(timestamp_)
          && buff->PutVersion(version_)
          && buff->PutLong(height_)
          && buff->PutHash(previous_hash_)
          && buff->PutSetOf(transactions_);
    }

    bool Write(Json::Writer& writer) const override{
      return writer.StartObject()
            && json::SetField(writer, "timestamp", timestamp_)
            && json::SetField(writer, "version", version_.ToString())
            && json::SetField(writer, "height", height_)
            && json::SetField(writer, "previous_hash", previous_hash_)
            && json::SetField(writer, "hash", GetHash())
            && json::SetField(writer, "transactions", GetNumberOfTransactions())
          && writer.EndObject();
    }

    bool Equals(const BlockPtr& blk) const{
      if(timestamp_ != blk->timestamp_){
        return false;
      }
      if(height_ != blk->height_){
        return false;
      }
      if(previous_hash_ != blk->previous_hash_){
        return false;
      }
      if(transactions_.size() != blk->transactions_.size()){
        return false;
      }
      return std::equal(
        transactions_.begin(),
        transactions_.end(),
        blk->transactions_.begin(),
        [](const TransactionPtr& a, const TransactionPtr& b){
          return a->Equals(b);
        }
      );
    }

    Hash GetMerkleRoot() const;
    bool Accept(BlockVisitor* vis) const;
    bool Contains(const Hash& hash) const;

    static BlockPtr Genesis();

    static inline BlockPtr
    NewInstance(int64_t height,
                const Version& version,
                const Hash& phash,
                const IndexedTransactionSet& transactions,
                Timestamp timestamp = Clock::now()){
      return std::make_shared<Block>(height, version, phash, transactions, timestamp);
    }

    static inline BlockPtr
    NewInstance(const BlockHeader& header, const IndexedTransactionSet& txs){
      return std::make_shared<Block>(
        header.GetHeight(),
        header.GetVersion(),
        header.GetPreviousHash(),
        txs,
        header.GetTimestamp()
      );
    }

    static inline BlockPtr
    FromParent(const BlockPtr& parent, const IndexedTransactionSet& txs, const Timestamp& timestamp = Clock::now()){
      return std::make_shared<Block>(parent, txs, timestamp);
    }

    static inline BlockPtr
    FromBytes(const BufferPtr& buff){
      Timestamp timestamp = FromUnixTimestamp(buff->GetLong());
      Version version = buff->GetVersion();
      int64_t height = buff->GetLong();
      Hash previous_hash = buff->GetHash();

      int64_t idx;
      int64_t num_transactions = buff->GetLong();
      IndexedTransactionSet transactions;
      for(idx = 0; idx < num_transactions; idx++)
        transactions.insert(IndexedTransaction::FromBytes(buff));

      return std::make_shared<Block>(height, version, previous_hash, transactions, timestamp);
    }

    static inline BlockPtr
    FromFile(const std::string& filename){
      BufferPtr buffer = Buffer::FromFile(filename);
      ObjectTag tag = buffer->GetObjectTag();
      if(!tag.IsBlockType()){
        LOG(WARNING) << "unexpected tag of " << tag << ", cannot read Block from: " << filename;
        return BlockPtr(nullptr);
      }
      return FromBytes(buffer);
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
}

#endif //TOKEN_BLOCK_H