#ifndef TOKEN_RAW_PROPOSAL_H
#define TOKEN_RAW_PROPOSAL_H

#include "uuid.h"

#include "codec.h"
#include "encoder.h"
#include "decoder.h"

#include "block_header.h"

namespace token{
class RawProposal : public Object{
 public:
    class Encoder : public codec::EncoderBase<RawProposal>{
     private:
      typedef codec::EncoderBase<RawProposal> BaseType;
     public:
      Encoder(const RawProposal& value, const codec::EncoderFlags& flags):
        BaseType(value, flags){}
      Encoder(const Encoder& other) = default;
      ~Encoder() override = default;

      int64_t GetBufferSize() const override{
        return 0;
      }

      bool Encode(const BufferPtr& buff) const override{
        return false;
      }

      Encoder& operator=(const Encoder& other) = default;
    };

    class Decoder : public codec::DecoderBase<RawProposal>{
     private:
      typedef codec::DecoderBase<RawProposal> BaseType;
     public:
      explicit Decoder(const codec::EncoderFlags& flags):
        BaseType(flags){}
      Decoder(const Decoder& other) = default;
      ~Decoder() override = default;

      bool Decode(const BufferPtr& buff, RawProposal& result) const override{
        return false;
      }

      Decoder& operator=(const Decoder& other) = default;
    };
 private:
  Timestamp timestamp_;
  UUID id_;
  UUID proposer_;
  BlockHeader value_;//TODO: serialize
 public:
  RawProposal():
    Object(),
    timestamp_(Clock::now()),
    id_(),
    proposer_(),
    value_(){}
  RawProposal(const Timestamp& timestamp,
              const UUID& id,
              const UUID& proposer,
              const BlockHeader& value):
    Object(),
    timestamp_(timestamp),
    id_(id),
    proposer_(proposer),
    value_(value){}
  ~RawProposal() override = default;

  Type type() const override{
    return Type::kProposal;
  }

  Timestamp& timestamp(){
    return timestamp_;
  }

  Timestamp timestamp() const{
    return timestamp_;
  }

  UUID& proposal_id(){
    return id_;
  }

  UUID proposal_id() const{
    return id_;
  }

  UUID& proposer_id(){
    return proposer_;
  }

  UUID proposer_id() const{
    return proposer_;
  }

  BlockHeader& value(){
    return value_;
  }

  BlockHeader value() const{
    return value_;
  }

  std::string ToString() const override{
    std::stringstream ss;
    ss << "Proposal(";
    ss << "timestamp=" << ToUnixTimestamp(timestamp_) << ", ";
    ss << "id=" << id_  << ", ";
    ss << "proposer=" << proposer_ << ", ";
    ss << ")";
    return ss.str();
  }

  RawProposal& operator=(const RawProposal& proposal) = default;

  friend bool operator==(const RawProposal& a, const RawProposal& b){
    return ToUnixTimestamp(a.timestamp_) == ToUnixTimestamp(b.timestamp_)
        && a.id_ == b.id_
        && a.proposer_ == b.proposer_;
  }

  friend bool operator!=(const RawProposal& a, const RawProposal& b){
    return !operator==(a, b);
  }

  friend bool operator<(const RawProposal& a, const RawProposal& b){
    return false;
  }

  friend bool operator>(const RawProposal& a, const RawProposal& b){
    return false;
  }

  friend std::ostream& operator<<(std::ostream& stream, const RawProposal& proposal){
    return stream << proposal.ToString();
  }

  static inline int64_t
  GetSize(){
    //TODO: use UUID encoder sizing
    int64_t size = 0;
    size += sizeof(RawTimestamp);
    size += 16;
    size += 16;
    return size;
  }
};
}

#endif//TOKEN_RAW_PROPOSAL_H