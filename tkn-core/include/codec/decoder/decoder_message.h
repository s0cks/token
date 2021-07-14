#ifndef TKN_CODEC_DECODER_MESSAGE_H
#define TKN_CODEC_DECODER_MESSAGE_H

namespace token{
  namespace codec{
  template<class M>
  class MessageDecoder : public codec::DecoderBase<M>{
   protected:
    explicit MessageDecoder(const codec::DecoderHints& hints):
        codec::DecoderBase<M>(hints){}
   public:
    MessageDecoder(const MessageDecoder<M>& other) = default;
    ~MessageDecoder() override = default;
    MessageDecoder<M>& operator=(const MessageDecoder<M>& other) = default;
  };
  }
}

#endif//TKN_CODEC_DECODER_MESSAGE_H