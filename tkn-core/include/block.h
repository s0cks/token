#ifndef TOKEN_BLOCK_H
#define TOKEN_BLOCK_H

#include "block.pb.h"

#include "bloom.h"
#include "version.h"
#include "builder.h"
#include "timestamp.h"
#include "transaction_indexed.h"
#include "block_header.h"

namespace token{
  class BlockVisitor;
  class Block: public BinaryObject{
    //TODO:
    // - validation logic
    friend class rpc::BlockMessage;
  public:
#ifdef TOKEN_DEBUG
    static const int64_t kMaxBlockSize = 128 * token::internal::kMegabytes;
    static const int64_t kNumberOfGenesisOutputs = 128;
#else
    static const int64_t kMaxBlockSize = 1 * token::internal::kGigabytes;
    static const int64_t kNumberOfGenesisOutputs = 10000;
#endif//TOKEN_DEBUG

    class Builder : public internal::ProtoBuilder<Block, internal::proto::Block>{
    public:
      Builder() = default;
      ~Builder() override = default;

      void SetTimestamp(const Timestamp& val){
        raw_->set_timestamp(ToUnixTimestamp(val));
      }

      void SetPreviousHash(const Hash& val){
        raw_->set_previous_hash(val.HexString());
      }

      void SetHeight(const uint64_t& height){
        raw_->set_height(height);
      }

      IndexedTransaction::Builder AddTransaction() const{
        return IndexedTransaction::Builder(raw_->add_transactions());
      }

      BlockPtr Build() const override{
        return std::make_shared<Block>(*raw_);
      }
    };
  private:
    internal::proto::Block raw_;

    BloomFilter tx_bloom_; // transient

    inline internal::proto::Block&
    raw(){
      return raw_;
    }
  public:
    Block():
      BinaryObject(),
      raw_(){}
    explicit Block(internal::proto::Block raw):
      BinaryObject(),
      raw_(std::move(raw)){}
    explicit Block(const internal::BufferPtr& data):
      Block(){
      if(!raw_.ParseFromArray(data->data(), static_cast<int>(data->length())))
        DLOG(FATAL) << "cannot parse Block from buffer.";
    }
    Block(const Block& other) = default;
    ~Block() override = default;

    Type type() const override{
      return Type::kBlock;
    }

    Hash hash() const override{
      auto data = ToBuffer();
      return Hash::ComputeHash<CryptoPP::SHA256>(data->data(), data->length());
    }

    Timestamp timestamp() const{
      return FromUnixTimestamp(raw_.timestamp());
    }

    uint64_t height() const{
      return raw_.height();
    }

    Hash previous_hash() const{
      return Hash::FromHexString(raw_.previous_hash());
    }

    BlockHeader GetHeader() const{
      return BlockHeader(
          timestamp(),
          height(),
          previous_hash(),
          GetMerkleRoot(),
          hash()
      );
    }

    BloomFilter& GetBloomFilter(){
      return tx_bloom_;
    }

    BloomFilter GetBloomFilter() const{
      return tx_bloom_;
    }

    uint64_t GetNumberOfTransactions() const{
      return raw_.transactions_size();
    }

    IndexedTransactionPtr GetTransaction(const uint64_t& index) const{
      return std::make_shared<IndexedTransaction>(raw_.transactions(static_cast<int>(index)));
    }

    bool IsGenesis() const{
      return height() == 0;
    }

    BufferPtr ToBuffer() const;
    std::string ToString() const override;

    Hash GetMerkleRoot() const;
    bool Accept(BlockVisitor* vis) const;
    bool Contains(const Hash& hash) const;

    void GetTransactions(IndexedTransactionSet& results){
      auto transactions = raw_.transactions();
      std::for_each(transactions.begin(), transactions.end(), [&](internal::proto::IndexedTransaction& val){
        results.insert(std::make_shared<IndexedTransaction>(val));
      });
    }

    bool VisitTransactions(IndexedTransactionVisitor* vis) const{
      auto transactions = raw_.transactions();
      for(auto& it : transactions){
        auto val = std::make_shared<IndexedTransaction>(it);
        if(!vis->Visit(val))
          return false;
      }
      return true;
    }

    template<typename Callback>
    bool ForEachTransaction(Callback callback){//TODO: remove
      std::for_each(raw_.transactions().begin(), raw_.transactions().end(), callback);
      return true;
    }

    friend bool operator==(const Block& lhs, const Block& rhs){
      return lhs.hash() == rhs.hash();
    }

    static inline BlockPtr
    NewInstance(internal::proto::Block raw){
      return std::make_shared<Block>(std::move(raw));
    }

    static inline BlockPtr
    Decode(const internal::BufferPtr& data){
      return std::make_shared<Block>(data);
    }

    static BlockPtr Genesis();
  };

  class BlockVisitor{
  protected:
    BlockVisitor() = default;
  public:
    virtual ~BlockVisitor() = default;
    virtual bool Visit(const BlockPtr& val) const = 0;
  };

  namespace json{
    static inline bool
    Write(Writer& writer, const BlockPtr& val){
      JSON_START_OBJECT(writer);
      {
        if(!json::SetField(writer, "timestamp", val->timestamp()))
          return false;
        if(!json::SetField(writer, "height", val->height()))
          return false;
        if(!json::SetField(writer, "previous_hash", val->previous_hash()))
          return false;
        if(!json::SetField(writer, "merkle_root", val->GetMerkleRoot()))
          return false;

        auto printer = [&](const internal::proto::IndexedTransaction& raw){
          IndexedTransaction val(raw);
          DLOG(INFO) << "visiting transaction: " << val.hash();
          JSON_STRING(writer, val.hash().HexString());
          return true;
        };

        if(!json::SetField(writer, "num_transactions", val->GetNumberOfTransactions()))
          return false;

        JSON_KEY(writer, "transactions");
        JSON_START_ARRAY(writer);
        {
          if(!val->ForEachTransaction(printer))
            return false;
        }
        JSON_END_ARRAY(writer);
      }
      JSON_END_OBJECT(writer);
      return true;
    }

    static inline bool
    SetField(Writer& writer, const char* name, const BlockPtr& val){
      JSON_KEY(writer, name);
      return Write(writer, val);
    }
  }
}
#endif //TOKEN_BLOCK_H