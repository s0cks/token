#ifndef TOKEN_RPC_MESSAGE_PAXOS_H
#define TOKEN_RPC_MESSAGE_PAXOS_H

#include "network/rpc_message.h"

namespace token{
  namespace rpc{
    class PaxosMessage;
    typedef std::shared_ptr<PaxosMessage> PaxosMessagePtr;

    class PaxosMessage : public rpc::Message{
     public:
      static inline int
      CompareProposal(const PaxosMessagePtr& a, const PaxosMessagePtr& b){
        RawProposal& p1 = a->proposal(), p2 = b->proposal();
        if(p1 < p2){
          return -1;
        } else if(p1 > p2){
          return +1;
        }
        return 0;
      }

      template<class M>
      class PaxosMessageEncoder : public MessageEncoder<M>{
       protected:
        RawProposal::Encoder proposal_encoder_;

        PaxosMessageEncoder(const M& value, const codec::EncoderFlags& flags):
          MessageEncoder<M>(value, flags),
          proposal_encoder_(value.proposal(), flags){}
       public:
        PaxosMessageEncoder(const PaxosMessageEncoder<M>& other) = default;
        ~PaxosMessageEncoder() override = default;

        int64_t GetBufferSize() const override{
          int64_t size = MessageEncoder<M>::GetBufferSize();
          size += proposal_encoder_.GetBufferSize();
          return size;
        }

        bool Encode(const BufferPtr& buff) const override{
          if(!MessageEncoder<M>::Encode(buff))
            return false;
          return proposal_encoder_.GetBufferSize();
        }

        PaxosMessageEncoder<M>& operator=(const PaxosMessageEncoder<M>& other) = default;
      };

      template<class M>
      class PaxosMessageDecoder : public codec::DecoderBase<M>{
       protected:
        RawProposal::Decoder proposal_decoder_;

        explicit PaxosMessageDecoder(const codec::DecoderHints& hints):
            codec::DecoderBase<M>(hints),
            proposal_decoder_(hints){}
       public:
        PaxosMessageDecoder(const PaxosMessageDecoder<M>& other) = default;
        ~PaxosMessageDecoder<M>() override = default;
        virtual bool Decode(const BufferPtr& buff, M& result) const = 0;
        PaxosMessageDecoder<M>& operator=(const PaxosMessageDecoder<M>& other) = default;
      };
     protected:
      RawProposal proposal_;

      PaxosMessage():
        rpc::Message(),
        proposal_(){}
      explicit PaxosMessage(const RawProposal& proposal):
        rpc::Message(),
        proposal_(proposal){}
     public:
      PaxosMessage(const PaxosMessage& other) = default;
      ~PaxosMessage() override = default;

      RawProposal& proposal(){
        return proposal_;
      }

      RawProposal proposal() const{
        return proposal_;
      }

      PaxosMessage& operator=(const PaxosMessage& other) = default;
    };
  }
}

#endif//TOKEN_RPC_MESSAGE_PAXOS_H