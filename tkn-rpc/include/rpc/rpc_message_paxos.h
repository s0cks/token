#ifndef TOKEN_RPC_MESSAGE_PAXOS_H
#define TOKEN_RPC_MESSAGE_PAXOS_H

#include "proposal.h"
#include "rpc/rpc_message.h"

namespace token{
  namespace rpc{
    class PaxosMessage;
    typedef std::shared_ptr<PaxosMessage> PaxosMessagePtr;

    class PaxosMessage : public rpc::Message{
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
     protected:
      std::shared_ptr<Proposal> proposal_;

      PaxosMessage():
        rpc::Message(),
        proposal_(){}
      explicit PaxosMessage(const Proposal& proposal):
        rpc::Message(),
        proposal_(std::make_shared<Proposal>(proposal)){}
     public:
      ~PaxosMessage() override = default;

      std::shared_ptr<Proposal> proposal() const{
        return proposal_;
      }
    };
  }

  namespace codec{
    template<class M>
    class PaxosMessageEncoder : public MessageEncoder<M>{
    protected:
      Proposal::Encoder encode_proposal_;

      PaxosMessageEncoder(const M* value, const codec::EncoderFlags& flags):
        MessageEncoder<M>(value, flags),
        encode_proposal_(value->proposal().get(), codec::kDefaultEncoderFlags){}//TODO: fix
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
      class PaxosMessageDecoder : public codec::MessageDecoder<M>{
      protected:
        Proposal::Decoder decode_proposal_;

        explicit PaxosMessageDecoder(const codec::DecoderHints& hints):
          codec::MessageDecoder<M>(hints),
          decode_proposal_(hints){}

        inline Proposal*
        DecodeProposal(const BufferPtr& data) const{
          return decode_proposal_.Decode(data);
        }
      public:
        ~PaxosMessageDecoder() override = default;
      };
  }
}

#endif//TOKEN_RPC_MESSAGE_PAXOS_H