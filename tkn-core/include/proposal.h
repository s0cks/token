#ifndef TOKEN_PROPOSAL_H
#define TOKEN_PROPOSAL_H

#include <memory>
#include <ostream>

#include "proposal.pb.h"

#include "builder.h"

#include "uuid.h"
#include "object.h"
#include "codec/codec.h"

namespace token{
  typedef internal::proto::Proposal RawProposal;
  class Proposal: public Object{
  public:
    static inline int
    Compare(const Proposal& lhs, const Proposal& rhs){
      NOT_IMPLEMENTED(FATAL);
      return 0;
    }

    class Builder : public internal::ProtoBuilder<Proposal, RawProposal>{
    public:
      Builder() = default;
      explicit Builder(RawProposal* raw):
        internal::ProtoBuilder<Proposal, RawProposal>(raw){}
      ~Builder() override = default;

      void SetTimestamp(const Timestamp& val){
        raw_->set_timestamp(ToUnixTimestamp(val));
      }

      void SetHeight(const uint64_t& val){
        raw_->set_height(val);
      }

      void SetHash(const Hash& val){
        raw_->set_hash(val.HexString());
      }

      ProposalPtr Build() const override{
        return std::make_shared<Proposal>(*raw_);
      }
    };
  private:
    internal::proto::Proposal raw_;
  public:
    Proposal():
      Object(),
      raw_(){}
    explicit Proposal(RawProposal raw):
      Object(),
      raw_(std::move(raw)){}
    ~Proposal() override = default;

    Type type() const override{
      return Type::kProposal;
    }

    Timestamp timestamp() const{
      return FromUnixTimestamp(raw_.timestamp());
    }

    UUID proposal_id() const{
      return UUID(raw_.proposal_id());
    }

    UUID proposer_id() const{
      return UUID(raw_.proposer_id());
    }

    uint64_t height() const{
      return raw_.height();
    }

    Hash hash() const{
      return Hash::FromHexString(raw_.hash());
    }

    std::string ToString() const override{
      NOT_IMPLEMENTED(ERROR);//TODO: implement
      return "Proposal()";
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