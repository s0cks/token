#ifndef TOKEN_BLOCK_H
#define TOKEN_BLOCK_H

#include "bloom.h"
#include "version.h"
#include "timestamp.h"
#include "block_header.h"
#include "binary_object.h"
#include "indexed_transaction.h"

#include "codec.h"
#include "encoder.h"
#include "decoder.h"

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

    class Encoder : public codec::EncoderBase<Block>{
     public:
      Encoder(const Block& value, const codec::EncoderFlags& flags=codec::kDefaultEncoderFlags):
        codec::EncoderBase<Block>(value, flags){}
      Encoder(const Encoder& other) = default;
      ~Encoder() override = default;

      int64_t GetBufferSize() const override;
      bool Encode(const BufferPtr& buff) const override;
      Encoder& operator=(const Encoder& other) = default;
    };

    class Decoder : public codec::DecoderBase<Block>{
     public:
      Decoder(const codec::DecoderHints& hints=codec::kDefaultDecoderHints):
        codec::DecoderBase<Block>(hints){}
      Decoder(const Decoder& other) = default;
      ~Decoder() override = default;
      bool Decode(const BufferPtr& buff, Block& result) const override;
      Decoder& operator=(const Decoder& other) = default;
    };
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
    Block(const Timestamp& timestamp,
          const int64_t& height,
          const Hash& previous_hash,
          const IndexedTransactionSet& transactions,
          const Version& version):
      BinaryObject(),
      timestamp_(timestamp),
      version_(version),
      height_(height),
      previous_hash_(previous_hash),
      transactions_(transactions),
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
          tx_bloom_.Put(it->hash());
      }
    }
    Block(const BlockPtr& parent, const IndexedTransactionSet& transactions, Timestamp timestamp = Clock::now()):
      Block(parent->height() + 1, Version(TOKEN_MAJOR_VERSION, TOKEN_MINOR_VERSION, TOKEN_REVISION_VERSION), parent->hash(), transactions, timestamp){}
    Block(const Block& other) = default;
    ~Block() override = default;

    Type type() const override{
      return Type::kBlock;
    }

    Timestamp& timestamp(){
      return timestamp_;
    }

    Timestamp timestamp() const{
      return timestamp_;
    }

    Version GetVersion() const{
      return version_;
    }

    int64_t& height(){
      return height_;
    }

    int64_t height() const{
      return height_;
    }

    Hash GetPreviousHash() const{
      return previous_hash_;
    }

    BlockHeader GetHeader() const{
      return BlockHeader(
          timestamp(),
          height(),
          GetPreviousHash(),
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
      return height() == 0;
    }

    BufferPtr ToBuffer() const override;

    std::string ToString() const override;

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
    FromParent(const BlockPtr& parent, const IndexedTransactionSet& txs, const Timestamp& timestamp = Clock::now()){
      return std::make_shared<Block>(parent, txs, timestamp);
    }

    static inline bool
    Decode(const BufferPtr& buff, Block& result, const codec::DecoderHints& hints=codec::kDefaultDecoderHints){
      Decoder decoder(hints);
      return decoder.Decode(buff, result);
    }

    static inline BlockPtr
    DecodeNew(const BufferPtr& buff, const codec::DecoderHints& hints=codec::kDefaultDecoderHints){
      Block result;
      if(!Decode(buff, result, hints)){
        DLOG(WARNING) << "cannot decode Block";
        return nullptr;
      }
      return std::make_shared<Block>(result);
    }
  };

  class BlockVisitor{
   protected:
    BlockVisitor() = default;
   public:
    virtual ~BlockVisitor() = default;
    virtual bool VisitStart(){ return true; }
    virtual bool Visit(const IndexedTransactionPtr& tx) = 0;
    virtual bool VisitEnd(){ return true; }
  };
}

#endif //TOKEN_BLOCK_H