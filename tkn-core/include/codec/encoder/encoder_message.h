#ifndef TKN_CODEC_ENCODER_MESSAGE_H
#define TKN_CODEC_ENCODER_MESSAGE_H

namespace token{
  namespace codec{
  template<class M>
  class MessageEncoder : public codec::EncoderBase<M>{
   protected:
    MessageEncoder(const M& value, const codec::EncoderFlags& flags):
        codec::EncoderBase<M>(value, flags){}
   public:
    MessageEncoder(const MessageEncoder<M>& other) = default;
    ~MessageEncoder() override = default;
    MessageEncoder& operator=(const MessageEncoder& other) = default;
  };
  }
}

#endif//TKN_CODEC_ENCODER_MESSAGE_H