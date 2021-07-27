#ifndef TOKEN_PROPOSAL_H
#define TOKEN_PROPOSAL_H

#include <memory>
#include <ostream>

#include "uuid.h"
#include "object.h"
#include "codec/codec.h"

namespace token{
  class Proposal: public Object{
  public:
    static inline int
    Compare(const Proposal& lhs, const Proposal& rhs){
      NOT_IMPLEMENTED(FATAL);
      return 0;
    }

    class Encoder: public codec::TypeEncoder<Proposal>{
    public:
      Encoder(const Proposal* value, const codec::EncoderFlags& flags);
      Encoder(const Encoder& other) = default;
      ~Encoder() override = default;
      int64_t GetBufferSize() const override;
      bool Encode(const BufferPtr& buff) const override;
      Encoder& operator=(const Encoder& other) = default;
    };

    class Decoder: public codec::DecoderBase<Proposal>{
    public:
      explicit Decoder(const codec::DecoderHints& hints);
      Decoder(const Decoder& other) = default;
      ~Decoder() override = default;
      bool Decode(const BufferPtr& buff, Proposal& result) const override;
      Decoder& operator=(const Decoder& other) = default;
    };
  private:
    Timestamp timestamp_;
    UUID proposal_id_;
    UUID proposer_id_;
    int64_t height_;
    Hash hash_;
  public:
    Proposal():
        Object(),
        timestamp_(),
        proposal_id_(),
        proposer_id_(),
        height_(),
        hash_(){}

    Proposal(const Timestamp& timestamp,
        const UUID& proposal_id,
        const UUID& proposer_id,
        const int64_t& height,
        const Hash& hash):
        Object(),
        timestamp_(timestamp),
        proposal_id_(proposal_id),
        proposer_id_(proposer_id),
        height_(height),
        hash_(hash){}

    ~Proposal() override = default;

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
      return proposal_id_;
    }

    UUID proposal_id() const{
      return proposal_id_;
    }

    UUID& proposer_id(){
      return proposer_id_;
    }

    UUID proposer_id() const{
      return proposer_id_;
    }

    int64_t& height(){
      return height_;
    }

    int64_t height() const{
      return height_;
    }

    Hash& hash(){
      return hash_;
    }

    Hash hash() const{
      return hash_;
    }

    std::string ToString() const override{
      std::stringstream ss;
      ss << "Proposal(";
      ss << "timestamp=" << FormatTimestampReadable(timestamp_) << ", ";
      ss << "proposal=" << proposal_id_ << ", ";
      ss << "proposer" << proposer_id_ << ", ";
      ss << "height=" << height_ << ", ";
      ss << "hash=" << hash_;
      ss << ")";
      return ss.str();
    }

    Proposal& operator=(const Proposal& rhs) = default;

    friend bool operator==(const Proposal& lhs, const Proposal& rhs){
      return Compare(lhs, rhs) == 0;
    }

    friend bool operator!=(const Proposal& lhs, const Proposal& rhs){
      return Compare(lhs, rhs) != 0;
    }

    friend bool operator<(const Proposal& lhs, const Proposal& rhs){
      return Compare(lhs, rhs) < 0;
    }

    friend bool operator>(const Proposal& lhs, const Proposal& rhs){
      return Compare(lhs, rhs) > 0;
    }

    friend std::ostream& operator<<(std::ostream& stream, const Proposal& val){
      return stream << val.ToString();
    }
  };
}
#endif//TOKEN_PROPOSAL_H