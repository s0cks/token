#ifndef TOKEN_RPC_MESSAGE_PAXOS_H
#define TOKEN_RPC_MESSAGE_PAXOS_H

#include <utility>

#include "proposal.h"
#include "rpc/rpc_message.h"

namespace token{
  namespace rpc{
    class PaxosMessage;
    typedef std::shared_ptr<PaxosMessage> PaxosMessagePtr;

    class PaxosMessage : public RawMessage<RawProposal>{
     public:
      static inline int
      CompareProposal(const PaxosMessagePtr& a, const PaxosMessagePtr& b){
        const Proposal& p1 = *a->proposal(), p2 = *b->proposal();
        if(p1 < p2){
          return -1;
        } else if(p1 > p2){
          return +1;
        }
        return 0;
      }

      template<class T>
      class PaxosMessageBuilderBase : public internal::ProtoBuilder<T, RawProposal>{
      public:
        explicit PaxosMessageBuilderBase(RawProposal* raw):
          internal::ProtoBuilder<T, RawProposal>(raw){}
        PaxosMessageBuilderBase() = default;
        ~PaxosMessageBuilderBase() = default;

        void SetTimestamp(const Timestamp& val){
          internal::ProtoBuilder<T, RawProposal>::raw()->set_timestamp(ToUnixTimestamp(val));
        }

        void SetHeight(const uint64_t& val){
          internal::ProtoBuilder<T, RawProposal>::raw()->set_height(val);
        }

        void SetHash(const Hash& val){
          internal::ProtoBuilder<T, RawProposal>::raw()->set_hash(val.HexString());
        }

        std::shared_ptr<T> Build() const override{
          return std::make_shared<T>(*internal::ProtoBuilder<T, RawProposal>::raw());
        }
      };
     protected:
      PaxosMessage() = default;
      explicit PaxosMessage(RawProposal raw):
        RawMessage<RawProposal>(std::move(raw)){}
      explicit PaxosMessage(const BufferPtr& data):
        RawMessage<RawProposal>(data){}
     public:
      ~PaxosMessage() override = default;

      Timestamp timestamp() const{
        return FromUnixTimestamp(RawMessage<RawProposal>::raw_.timestamp());
      }

      uint64_t height() const{
        return RawMessage<RawProposal>::raw_.height();
      }

      ProposalPtr proposal() const{
        return std::make_shared<Proposal>(raw_);
      }
    };
  }
}

#endif//TOKEN_RPC_MESSAGE_PAXOS_H