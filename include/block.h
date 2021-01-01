#ifndef TOKEN_BLOCK_H
#define TOKEN_BLOCK_H

#include <json/json.h>
#include "hash.h"
#include "bloom.h"
#include "transaction.h"
#include "utils/buffer.h"

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
    BlockHeader(Timestamp timestamp,
                int64_t height,
                const Hash& phash,
                const Hash& merkle_root,
                const Hash& hash,
                const BloomFilter& tx_bloom):
      timestamp_(timestamp),
      height_(height),
      previous_hash_(phash),
      merkle_root_(merkle_root),
      hash_(hash),
      bloom_(tx_bloom){}
    BlockHeader(const BufferPtr& buff);
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
    bool Write(const BufferPtr& buff) const;

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
    static const int64_t kNumberOfGenesisOutputs = 1000; // TODO: changeme

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

    enum RawBlockLayout{
      kTimestampOffset = 0,
      kTimestampSize = 64,

      kHeightOffset = kTimestampSize + kTimestampOffset,
      kHeightSize = 64,

      kPreviousHashOffset = kHeightOffset + kHeightSize,
      kPreviousHashSize = Hash::kSize,

      kTransactionListOffset = kPreviousHashOffset + kPreviousHashSize,
    };
   private:
    Timestamp timestamp_;
    int64_t height_;
    Hash previous_hash_;
    TransactionList transactions_;
    BloomFilter tx_bloom_; // transient
   public:
    Block():
      BinaryObject(),
      timestamp_(0),
      height_(0),
      previous_hash_(),
      transactions_(),
      tx_bloom_(){}
    Block(int64_t height,
          const Hash& phash,
          const TransactionList& transactions,
          Timestamp timestamp = GetCurrentTimestamp()):
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
    Block(const BlockPtr& parent, const TransactionList& transactions, Timestamp timestamp = GetCurrentTimestamp()):
      Block(parent->GetHeight() + 1, parent->GetHash(), transactions, timestamp){}
    Block(const BlockHeader& parent, const TransactionList& transactions, Timestamp timestamp = GetCurrentTimestamp()):
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
      size += sizeof(Timestamp); // timestamp_
      size += sizeof(int64_t); // height_
      size += Hash::GetSize(); // previous_hash_
      size += sizeof(int64_t); // num_transactions
      for(auto& it : transactions_)
        size += it->GetBufferSize();
      return size;
    }

    bool Write(const BufferPtr& buff) const{
      buff->PutLong(timestamp_);
      buff->PutLong(height_);
      buff->PutHash(previous_hash_);
      buff->PutSet(transactions_);
      return true;
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

    static BlockPtr NewInstance(const BufferPtr& buff){
      Timestamp timestamp = buff->GetLong();
      int64_t height = buff->GetLong();
      Hash previous_hash = buff->GetHash();

      int64_t idx;
      int64_t num_transactions = buff->GetLong();
      TransactionList transactions;
      for(idx = 0; idx < num_transactions; idx++)
        transactions.insert(Transaction::NewInstance(buff));

      return std::make_shared<Block>(height, previous_hash, transactions, timestamp);
    }

    static inline BlockPtr
    NewInstance(const BlockPtr& parent, const TransactionList& txs, const Timestamp& timestamp = GetCurrentTimestamp()){
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
}

#endif //TOKEN_BLOCK_H