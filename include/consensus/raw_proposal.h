#ifndef TOKEN_RAW_PROPOSAL_H
#define TOKEN_RAW_PROPOSAL_H

#include "block_header.h"

namespace token{
  class RawProposal{
   private:
    Timestamp timestamp_;
    UUID id_;
    UUID proposer_;
    BlockHeader value_;
   public:
    RawProposal(const Timestamp& timestamp,
                const UUID& id,
                const UUID& proposer,
                const BlockHeader& value):
      timestamp_(timestamp),
      id_(id),
      proposer_(proposer),
      value_(value){}
    ~RawProposal() = default;

    Timestamp& GetTimestamp(){
      return timestamp_;
    }

    Timestamp GetTimestamp() const{
      return timestamp_;
    }

    UUID& GetID(){
      return id_;
    }

    UUID GetID() const{
      return id_;
    }

    UUID& GetProposer(){
      return proposer_;
    }

    UUID GetProposer() const{
      return proposer_;
    }

    BlockHeader& GetValue(){
      return value_;
    }

    BlockHeader GetValue() const{
      return value_;
    }

    bool Encode(const BufferPtr& buff) const{
      SERIALIZE_BASIC_FIELD(timestamp_, Timestamp);
      SERIALIZE_BASIC_FIELD(id_, UUID);
      SERIALIZE_BASIC_FIELD(proposer_, UUID);
      SERIALIZE_FIELD(value, BlockHeader, value_);
      return true;
    }

    int64_t GetBufferSize() const{
      return GetSize();
    }

    std::string ToString() const{
      std::stringstream ss;
      ss << "Proposal(";
      ss << "timestamp=" << FormatTimestampReadable(timestamp_) << ", ";
      ss << "id=" << id_  << ", ";
      ss << "proposer=" << proposer_ << ", ";
      ss << "value=" << value_;
      ss << ")";
      return ss.str();
    }

    void operator=(const RawProposal& proposal){
      timestamp_ = proposal.timestamp_;
      id_ = proposal.id_;
      proposer_ = proposal.proposer_;
      value_ = proposal.value_;
    }

    friend bool operator==(const RawProposal& a, const RawProposal& b){
      return false;
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
      int64_t size = 0;
      size += sizeof(RawTimestamp);
      size += UUID::GetSize();
      size += UUID::GetSize();
      size += BlockHeader::GetSize();
      return size;
    }
  };
}

#endif//TOKEN_RAW_PROPOSAL_H