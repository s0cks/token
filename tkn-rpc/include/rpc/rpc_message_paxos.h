#ifndef TOKEN_RPC_MESSAGE_PAXOS_H
#define TOKEN_RPC_MESSAGE_PAXOS_H

#include "type/proposal.h"
#include "rpc/rpc_message.h"

namespace token{
  namespace rpc{
    class PaxosMessage;
    typedef std::shared_ptr<PaxosMessage> PaxosMessagePtr;

    class PaxosMessage : public rpc::Message{
     public:
      static inline int
      CompareProposal(const PaxosMessagePtr& a, const PaxosMessagePtr& b){
        Proposal& p1 = a->proposal(), p2 = b->proposal();
        if(p1 < p2){
          return -1;
        } else if(p1 > p2){
          return +1;
        }
        return 0;
      }
     protected:
      Proposal proposal_;

      PaxosMessage():
        rpc::Message(),
        proposal_(){}
      explicit PaxosMessage(const Proposal& proposal):
        rpc::Message(),
        proposal_(proposal){}
     public:
      PaxosMessage(const PaxosMessage& other) = default;
      ~PaxosMessage() override = default;

      Proposal& proposal(){
        return proposal_;
      }

      Proposal proposal() const{
        return proposal_;
      }

      PaxosMessage& operator=(const PaxosMessage& other) = default;
    };
  }

  namespace codec{
    template<class M>
    class PaxosMessageEncoder : public MessageEncoder<M>{
    protected:
      codec::ProposalEncoder encode_proposal_;

      PaxosMessageEncoder(const M& value, const codec::EncoderFlags& flags):
        MessageEncoder<M>(value, flags),
        encode_proposal_(value.proposal(), flags){}
    public:
        PaxosMessageEncoder(const PaxosMessageEncoder<M>& other) = default;
        ~PaxosMessageEncoder() override = default;

        int64_t GetBufferSize() const override{
          auto size = MessageEncoder<M>::GetBufferSize();
          size += encode_proposal_.GetBufferSize();
          return size;
        }

        bool Encode(const BufferPtr& buff) const override{
          if(!MessageEncoder<M>::Encode(buff))
            return false;
          if(!encode_proposal_.Encode(buff)){
            LOG(FATAL) << "cannot encode proposal to buffer.";
            return false;
          }
          return true;
        }

        PaxosMessageEncoder<M>& operator=(const PaxosMessageEncoder<M>& other) = default;
    };

      template<class M>
      class PaxosMessageDecoder : public codec::DecoderBase<M>{
      protected:
        ProposalDecoder decode_proposal_;

        explicit PaxosMessageDecoder(const codec::DecoderHints& hints):
          codec::DecoderBase<M>(hints),
          decode_proposal_(hints){}

        inline bool
        DecodeProposalData(const BufferPtr& buff, Proposal& proposal) const{
          if(!decode_proposal_.Decode(buff, proposal)){
            LOG(FATAL) << "cannot decode proposal from buffer.";
            return false;
          }
          return true;
        }
      public:
          PaxosMessageDecoder(const PaxosMessageDecoder<M>& other) = default;
          ~PaxosMessageDecoder<M>() override = default;
          virtual bool Decode(const BufferPtr& buff, M& result) const = 0;
          PaxosMessageDecoder<M>& operator=(const PaxosMessageDecoder<M>& other) = default;
      };
  }
}

#endif//TOKEN_RPC_MESSAGE_PAXOS_H