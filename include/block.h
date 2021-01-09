#ifndef TOKEN_BLOCK_H
#define TOKEN_BLOCK_H

#include "hash.h"
#include "bloom.h"
#include "transaction.h"
#include "utils/buffer.h"
#include "utils/file_reader.h"
#include "utils/file_writer.h"

namespace Token{
  class Block;
  typedef std::shared_ptr<Block> BlockPtr;

  class BlockHeader{
   private:
    Timestamp timestamp_;
    int64_t height_;
    Hash previous_hash_;
    Hash merkle_root_;
    Hash hash_;
    BloomFilter bloom_;
    int64_t num_transactions_;
   public:
    BlockHeader():
      timestamp_(0),
      height_(0),
      previous_hash_(),
      merkle_root_(), // fill w/ genesis's merkle root
      hash_(), //TODO: fill w/ genesis's Hash
      bloom_(),
      num_transactions_(0){}
    BlockHeader(const BlockHeader& blk):
      timestamp_(blk.timestamp_),
      height_(blk.height_),
      previous_hash_(blk.previous_hash_),
      merkle_root_(blk.merkle_root_),
      hash_(blk.hash_),
      bloom_(blk.bloom_),
      num_transactions_(blk.num_transactions_){}
    BlockHeader(Timestamp timestamp,
      int64_t height,
      const Hash& phash,
      const Hash& merkle_root,
      const Hash& hash,
      const BloomFilter& tx_bloom,
      int64_t num_transactions):
      timestamp_(timestamp),
      height_(height),
      previous_hash_(phash),
      merkle_root_(merkle_root),
      hash_(hash),
      bloom_(tx_bloom),
      num_transactions_(num_transactions){}
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

    int64_t GetNumberOfTransactions() const{
      return num_transactions_;
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
    int64_t height_;
    Hash previous_hash_;
    TransactionSet transactions_;
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
      const TransactionSet& transactions,
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
    Block(const BlockPtr& parent, const TransactionSet& transactions, Timestamp timestamp = GetCurrentTimestamp()):
      Block(parent->GetHeight() + 1, parent->GetHash(), transactions, timestamp){}
    Block(const BlockHeader& parent, const TransactionSet& transactions, Timestamp timestamp = GetCurrentTimestamp()):
      Block(parent.GetHeight() + 1, parent.GetHash(), transactions, timestamp){}
    ~Block() = default;

    BlockHeader GetHeader() const{
      return BlockHeader(timestamp_, height_, previous_hash_, GetMerkleRoot(), GetHash(), tx_bloom_, transactions_.size());
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

    ObjectTag tag() const{
      return ObjectTag(ObjectTag::kBlock);
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
      return buff->PutLong(timestamp_)
             && buff->PutLong(height_)
             && buff->PutHash(previous_hash_)
             && buff->PutSet(transactions_);
    }

    bool Write(BinaryFileWriter* writer) const{
      return writer->WriteLong(timestamp_)
             && writer->WriteLong(height_)
             && writer->WriteHash(previous_hash_)
             && writer->WriteSet(transactions_);
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
    bool WriteToFile(const std::string& filename) const;

    static BlockPtr Genesis();

    static BlockPtr FromBytes(const BufferPtr& buff){
      Timestamp timestamp = buff->GetLong();
      int64_t height = buff->GetLong();
      Hash previous_hash = buff->GetHash();

      int64_t idx;
      int64_t num_transactions = buff->GetLong();
      TransactionSet transactions;
      for(idx = 0; idx < num_transactions; idx++)
        transactions.insert(Transaction::FromBytes(buff));

      return std::make_shared<Block>(height, previous_hash, transactions, timestamp);
    }

    static BlockPtr NewInstance(BinaryFileReader* reader){
      Timestamp timestamp = reader->ReadLong();
      int64_t height = reader->ReadLong();
      Hash phash = reader->ReadHash();

      int64_t idx;
      int64_t num_transactions = reader->ReadLong();
      TransactionSet transactions;
      for(idx = 0; idx < num_transactions; idx++)
        transactions.insert(Transaction::NewInstance(reader));
      return std::make_shared<Block>(height, phash, transactions, timestamp);
    }

    static inline BlockPtr
    NewInstance(const BlockPtr& parent, const TransactionSet& txs, const Timestamp& timestamp = GetCurrentTimestamp()){
      return std::make_shared<Block>(parent, txs, timestamp);
    }

    static BlockPtr FromFile(const std::string& filename);
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

  class BlockFileReader : BinaryObjectFileReader{
   public:
    BlockFileReader(const std::string& filename):
      BinaryObjectFileReader(filename, ObjectTag::kBlock){}
    ~BlockFileReader() = default;

    BlockPtr ReadBlock(){
      if(!ValidateTag()){
        LOG(WARNING) << "couldn't validate block tag.";
        return BlockPtr(nullptr);
      }
      return Block::NewInstance(this);
    }
  };
}

#endif //TOKEN_BLOCK_H