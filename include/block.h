#ifndef TOKEN_BLOCK_H
#define TOKEN_BLOCK_H

#include "hash.h"
#include "bloom.h"
#include "transaction.h"
#include "utils/buffer.h"

namespace token{
  class Block;
  typedef std::shared_ptr<Block> BlockPtr;

  class BlockVisitor;
  class Block : public BinaryObject, public std::enable_shared_from_this<Block>{
    //TODO:
    // - validation logic
    friend class BlockHeader;
    friend class BlockChain;
    friend class BlockMessage;
   public:
    static const int64_t kMaxTransactionsForBlock = 2;
    static const int64_t kNumberOfGenesisOutputs = 10000; // TODO: changeme
   private:
    Timestamp timestamp_;
    Version version_;
    int64_t height_;
    Hash previous_hash_;
    TransactionSet transactions_;
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
      const TransactionSet& transactions,
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
    Block(const BlockPtr& parent, const TransactionSet& transactions, Timestamp timestamp = Clock::now()):
      Block(parent->GetHeight() + 1, Version(TOKEN_MAJOR_VERSION, TOKEN_MINOR_VERSION, TOKEN_REVISION_VERSION), parent->GetHash(), transactions, timestamp){}
    ~Block() = default;

    Type GetType() const{
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

    int64_t GetNumberOfTransactions() const{
      return transactions_.size();
    }

    TransactionSet& transactions(){
      return transactions_;
    }

    TransactionSet transactions() const{
      return transactions_;
    }

    TransactionSet::iterator transactions_begin(){
      return transactions_.begin();
    }

    TransactionSet::const_iterator transactions_begin() const{
      return transactions_.begin();
    }

    TransactionSet::iterator transactions_end(){
      return transactions_.end();
    }

    TransactionSet::const_iterator transactions_end() const{
      return transactions_.end();
    }

    bool IsGenesis(){
      return GetHeight() == 0;
    }

    std::string ToString() const{
      std::stringstream stream;
      stream << "Block(#" << GetHeight() << ", " << GetNumberOfTransactions() << " Transactions)";
      return stream.str();
    }

    int64_t GetBufferSize() const{
      int64_t size = 0;
      size += sizeof(uint64_t); // timestamp_
      size += sizeof(RawVersion); // version_
      size += sizeof(int64_t); // height_
      size += Hash::GetSize(); // previous_hash_
      size += sizeof(int64_t); // num_transactions
      for(auto& it : transactions_)
        size += it->GetBufferSize();
      return size;
    }

    bool Write(const BufferPtr& buff) const{
      return buff->PutTimestamp(timestamp_)
          && buff->PutVersion(version_)
          && buff->PutLong(height_)
          && buff->PutHash(previous_hash_)
          && buff->PutSet(transactions_);
    }

    bool Write(Json::Writer& writer) const{
      return writer.StartObject()
            && Json::SetField(writer, "timestamp", timestamp_)
            && Json::SetField(writer, "version", version_.ToString())
            && Json::SetField(writer, "height", height_)
            && Json::SetField(writer, "previous_hash", previous_hash_)
            && Json::SetField(writer, "hash", GetHash())
            && Json::SetField(writer, "transactions", GetNumberOfTransactions())
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
    NewInstance(const BlockPtr& parent, const TransactionSet& txs, const Timestamp& timestamp = Clock::now()){
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
      TransactionSet transactions;
      for(idx = 0; idx < num_transactions; idx++)
        transactions.insert(Transaction::FromBytes(buff));

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