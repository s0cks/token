#ifndef TOKEN_BLOCK_H
#define TOKEN_BLOCK_H

#include "token/timestamp.h"
#include "token/transaction.h"

namespace token{
 class Block;
 class BlockVisitor{
  protected:
   BlockVisitor() = default;
   virtual ~BlockVisitor() = default;
   virtual void Visit(const Block& blk) const = 0;
 };

 class Block : public BinaryObject{
  private:
   uint64_t height_;
   Timestamp timestamp_;
   IndexedTransaction* transactions_;
   uint64_t num_transactions_;
  public:
   Block() = default;
   Block(uint64_t height, const Timestamp& timestamp, IndexedTransaction* transactions, uint64_t num_transactions):
    height_(height),
    timestamp_(timestamp),
    transactions_(new IndexedTransaction[num_transactions]),
    num_transactions_(num_transactions){
     std::copy(transactions, transactions + num_transactions, transactions_begin());
   }
   ~Block(){
     delete[] transactions_;
   }

   uint64_t height() const{
     return height_;
   }

   Timestamp timestamp() const{
     return timestamp_;
   }

   Transaction* transactions() const{
     return transactions_;
   }

   uint64_t GetNumberOfTransactions() const{
     return num_transactions_;
   }

   Transaction* transactions_begin() const{
     return &transactions()[0];
   }

   Transaction* transactions_end() const{
     return &transactions()[GetNumberOfTransactions()];
   }

   uint64_t GetBufferSize() const override{
     auto size = 0;
     size += sizeof(uint64_t); // height_
     size += sizeof(uint64_t); // timestamp_
     size += sizeof(uint64_t); // num_transactions_
     std::for_each(transactions_begin(), transactions_end(), [&](const Transaction& val){
       size += val.GetBufferSize();
     });
     return size;
   }

   bool WriteTo(const BufferPtr& data) const override{
     if(!data->PutUnsignedLong(height()))
       return false;
     if(!data->PutTimestamp(timestamp()))
       return false;
     if(!data->PutList(transactions(), GetNumberOfTransactions()))
       return false;
     return true;
   }

   Block& operator=(const Block& rhs){
     if(&rhs == this)
       return *this;
     height_ = rhs.height();
     timestamp_ = rhs.timestamp();

     if(transactions_ != nullptr)
       delete[] transactions_;
     transactions_ = new IndexedTransaction[rhs.GetNumberOfTransactions()];
     std::copy(rhs.transactions_begin(), rhs.transactions_end(), transactions_begin());
     return *this;
   }

   friend bool operator==(const Block& lhs, const Block& rhs){
     return lhs.hash() == rhs.hash();
   }

   friend bool operator!=(const Block& lhs, const Block& rhs){
     return lhs.hash() != rhs.hash();
   }
 };

//  class BlockVisitor{
//  protected:
//    BlockVisitor() = default;
//  public:
//    virtual ~BlockVisitor() = default;
//    virtual bool Visit(const BlockPtr& val) const = 0;
//  };
//
//  typedef internal::proto::Block RawBlock;
//  class Block: public BinaryObject{
//    //TODO:
//    // - validation logic
//    friend class rpc::BlockMessage;
//  public:
//#ifdef TOKEN_DEBUG
//    static const int64_t kMaxBlockSize = 128 * token::internal::kMegabytes;
//    static const int64_t kNumberOfGenesisOutputs = 10000;
//#else
//    static const int64_t kMaxBlockSize = 1 * token::internal::kGigabytes;
//    static const int64_t kNumberOfGenesisOutputs = 10000;
//#endif//TOKEN_DEBUG
//
//    static inline int
//    CompareHash(const Block& lhs, const Block& rhs){
//      return Hash::Compare(lhs.hash(), rhs.hash());
//    }
//
//    struct HashComparator{
//      bool operator()(const BlockPtr& lhs, const BlockPtr& rhs){
//        return CompareHash(*lhs, *rhs) < 0;
//      }
//    };
//
//    static inline int
//    CompareHeight(const Block& lhs, const Block& rhs){
//      if(lhs.height() < rhs.height()){
//        return -1;
//      } else if(lhs.height() > rhs.height()){
//        return +1;
//      }
//      return 0;
//    }
//
//    struct HeightComparator{
//      bool operator()(const BlockPtr& lhs, const BlockPtr& rhs){
//        return CompareHeight(*lhs, *rhs);
//      }
//    };
//
//    static inline int
//    CompareTimestamp(const Block& lhs, const Block& rhs){
//      if(lhs.timestamp() < rhs.timestamp()){
//        return -1;
//      } else if(lhs.timestamp() > rhs.timestamp()){
//        return +1;
//      }
//      return 0;
//    }
//
//    struct TimestampComparator{
//      bool operator()(const BlockPtr& lhs, const BlockPtr& rhs){
//        return CompareTimestamp(*lhs, *rhs);
//      }
//    };
//  private:
//    uint64_t height_;
//    Hash previous_;
//    Timestamp timestamp_;
//    IndexedTransactionSet transactions_;
//  public:
//    Block():
//      BinaryObject(),
//      height_(),
//      previous_(),
//      timestamp_(),
//      transactions_(){}
//    Block(const uint64_t& height, const Hash& previous, const Timestamp& timestamp, const IndexedTransactionSet& transactions):
//      BinaryObject(),
//      height_(height),
//      previous_(previous),
//      timestamp_(timestamp),
//      transactions_(transactions.begin(), transactions.end()){}
//    Block(const RawBlock& raw):
//      BinaryObject(){
//      NOT_IMPLEMENTED(ERROR);//TODO: implement
//    }
//    Block(const Block& other) = default;
//    ~Block() override = default;
//
//    Type type() const override{
//      return Type::kBlock;
//    }
//
//    uint64_t height() const{
//      return height_;
//    }
//
//    Hash previous() const{
//      return previous_;
//    }
//
//    Timestamp timestamp() const{
//      return timestamp_;
//    }
//
//    IndexedTransactionSet transactions() const{
//      return transactions_;
//    }
//
//    IndexedTransactionSet::iterator transactions_begin(){
//      return transactions_.begin();
//    }
//
//    IndexedTransactionSet::const_iterator transactions_begin() const{
//      return transactions_.begin();
//    }
//
//    IndexedTransactionSet::iterator transactions_end(){
//      return transactions_.end();
//    }
//
//    IndexedTransactionSet::const_iterator transactions_end() const{
//      return transactions_.end();
//    }
//
//    uint64_t GetNumberOfTransactions() const{
//      return transactions_.size();
//    }
//
//    bool VisitTransactions(IndexedTransactionVisitor* vis){
//      for(auto& it : transactions_){
//        if(!vis->Visit(it))
//          return false;
//      }
//      return true;
//    }
//
//    bool IsGenesis() const{
//      return height() == 0;
//    }
//
//    Hash GetMerkleRoot() const;
//
//    Hash hash() const override{
//      auto data = ToBuffer();
//      return Hash::ComputeHash<CryptoPP::SHA256>(data->data(), data->length());
//    }
//
//    std::string ToString() const override{
//      std::stringstream ss;
//      ss << "Block(";
//      ss << "height=" << height() << ", ";
//      ss << "timestamp=" << FormatTimestampReadable(timestamp()) << ", ";
//      ss << "transactions=" << transactions().size();//TODO: fix
//      ss << ")";
//      return ss.str();
//    }
//
//    internal::BufferPtr ToBuffer() const;
//
//    Block& operator=(const Block& rhs) = default;
//
//    Block& operator=(const RawBlock& rhs){
//      NOT_IMPLEMENTED(ERROR);//TODO: implement
//      return *this;
//    }
//
//    friend std::ostream& operator<<(std::ostream& stream, const Block& val){
//      return stream << val.ToString();
//    }
//
//    friend bool operator==(const Block& lhs, const Block& rhs){
//      return CompareHash(lhs, rhs);
//    }
//
//    friend bool operator!=(const Block& lhs, const Block& rhs){
//      return CompareHash(lhs, rhs);
//    }
//
//    friend bool operator<(const Block& lhs, const Block& rhs){
//      return CompareHeight(lhs, rhs);
//    }
//
//    friend bool operator>(const Block& lhs, const Block& rhs){
//      return CompareHeight(lhs, rhs);
//    }
//
//    static inline BlockPtr
//    NewInstance(const uint64_t& height, const Hash& previous, const Timestamp& timestamp, const IndexedTransactionSet& transactions){
//      return std::make_shared<Block>(height, previous, timestamp, transactions);
//    }
//
//    static inline BlockPtr
//    From(const RawBlock& val){
//      return std::make_shared<Block>(val);
//    }
//
//    static inline BlockPtr
//    From(const internal::BufferPtr& val){
//      auto length = val->GetUnsignedLong();
//      RawBlock raw;
//      if(!val->GetMessage(raw, length)){
//        LOG(FATAL) << "cannot decoded Block from " << val->ToString() << ".";
//        return nullptr;
//      }
//      return From(raw);
//    }
//
//    static inline BlockPtr
//    CopyFrom(const Block& val){
//      return std::make_shared<Block>(val);
//    }
//
//    static inline BlockPtr
//    CopyFrom(const BlockPtr& val){
//      return std::make_shared<Block>(*val);
//    }
//
//    static BlockPtr NewGenesis();
//  };
}
#endif //TOKEN_BLOCK_H