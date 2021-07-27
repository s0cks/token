#ifndef TOKEN_BLOCK_HEADER_H
#define TOKEN_BLOCK_HEADER_H

#include "hash.h"
#include "object.h"
#include "timestamp.h"

namespace token{
  class BlockHeader : public Object{
   public:
    class Encoder : public codec::TypeEncoder<BlockHeader>{
     public:
      Encoder(const BlockHeader* value, const codec::EncoderFlags& flags):
        codec::TypeEncoder<BlockHeader>(value, flags){}
      Encoder(const Encoder& other) = default;
      ~Encoder() override = default;
      int64_t GetBufferSize() const override;
      bool Encode(const BufferPtr& buff) const override;
      Encoder& operator=(const Encoder& other) = default;
    };

    class Decoder : public codec::DecoderBase<BlockHeader>{
     public:
      explicit Decoder(const codec::DecoderHints& hints):
        codec::DecoderBase<BlockHeader>(hints){}
      Decoder(const Decoder& other) = default;
      ~Decoder() override = default;
      bool Decode(const BufferPtr& buff, BlockHeader& result) const override;
      Decoder& operator=(const Decoder& other) = default;
    };
   protected:
    Timestamp timestamp_;
    int64_t height_;
    Hash previous_hash_;
    Hash merkle_root_;
    Hash hash_;
   public:
    BlockHeader():
      timestamp_(Clock::now()),
      height_(0),
      previous_hash_(),
      merkle_root_(),
      hash_(){}
    BlockHeader(const Timestamp& timestamp,
                const int64_t& height,
                const Hash& previous_hash,
                const Hash& merkle_root,
                const Hash& hash):
      timestamp_(timestamp),
      height_(height),
      previous_hash_(previous_hash),
      merkle_root_(merkle_root),
      hash_(hash){}
    BlockHeader(const BlockHeader& other) = default;
    ~BlockHeader() override = default;

    Type type() const override{
      return Type::kBlockHeader;
    }

    Timestamp& timestamp(){
      return timestamp_;
    }

    Timestamp timestamp() const{
      return timestamp_;
    }

    int64_t& height(){
      return height_;
    }

    int64_t height() const{
      return height_;
    }

    Hash& previous_hash(){
      return previous_hash_;
    }

    Hash previous_hash() const{
      return previous_hash_;
    }

    Hash& merkle_root(){
      return merkle_root_;
    }

    Hash merkle_root() const{
      return merkle_root_;
    }

    Hash& hash(){
      return hash_;
    }

    Hash hash() const{
      return hash_;
    }

    std::string ToString() const override{
      std::stringstream ss;
      ss << "BlockHeader()";
      return ss.str();
    }

    BlockHeader& operator=(const BlockHeader& other) = default;

    friend std::ostream& operator<<(std::ostream& stream, const BlockHeader& val){
      return stream << val.ToString();
    }
  };
}

#endif//TOKEN_BLOCK_HEADER_H