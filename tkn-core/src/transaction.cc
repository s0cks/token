/*
#include "pool.h"
#include "keychain.h"
#include "blockchain.h"
#include "transaction.h"
#include "server.h"
#include "unclaimed_transaction.h"

namespace token{
  std::string Transaction::ToString() const{
    std::stringstream ss;
    ss << "Transaction(";
    ss << "timestamp=" << FormatTimestampReadable(timestamp_) << ", ";
    ss << "inputs=[]" << ", "; //TODO: implement
    ss << "outputs=[]" << ","; //TODO: implement
    ss << ")";
    return ss.str();
  }

  BufferPtr Transaction::ToBuffer() const{
    Encoder encoder((*this));
    BufferPtr buffer = Buffer::AllocateFor(encoder);
    if(!encoder.Encode(buffer)){
      DLOG(ERROR) << "cannot encoder Transaction.";
      buffer->clear();
      return buffer;
    }
    return buffer;
  }

  bool Transaction::Sign(){
    CryptoPP::SecByteBlock bytes;
    */
/*
      TODO:
        if(!WriteMessage(bytes)){
            LOG(WARNING) << "couldn't serialize transaction to bytes";
            return false;
        }
     *//*


    CryptoPP::RSA::PrivateKey privateKey;
    CryptoPP::RSA::PublicKey publicKey;
    Keychain::LoadKeys(&privateKey, &publicKey);

    try{
      LOG(INFO) << "signing transaction: " << hash();
      CryptoPP::RSASS<CryptoPP::PSS, CryptoPP::SHA256>::Signer signer(privateKey);
      CryptoPP::AutoSeededRandomPool rng;
      CryptoPP::SecByteBlock sigData(signer.MaxSignatureLength());

      size_t length = signer.SignMessage(rng, bytes.data(), bytes.size(), sigData);
      sigData.resize(length);

      std::string signature;
      CryptoPP::ArraySource
        source(sigData.data(), sigData.size(), true, new CryptoPP::HexEncoder(new CryptoPP::StringSink(signature)));

      LOG(INFO) << "signature: " << signature;
      signature_ = signature;
      return true;
    } catch(CryptoPP::Exception& ex){
      LOG(ERROR) << "error occurred signing transaction: " << ex.GetWhat();
      return false;
    }
  }

  int64_t Transaction::Encoder::GetBufferSize() const{
    int64_t size = codec::EncoderBase<Transaction>::GetBufferSize();
    size += sizeof(RawTimestamp);
    size += codec::EncoderBase<Transaction>::GetBufferSize<Input, Input::Encoder>(value().inputs());
    size += codec::EncoderBase<Transaction>::GetBufferSize<Output, Output::Encoder>(value().outputs());
    return size;
  }

  template<class V, class E>
  static inline bool
  EncodeList(const BufferPtr& buff, const std::vector<V>& list, const codec::EncoderFlags& flags=codec::kDefaultEncoderFlags){
    if(!buff->PutLong(list.size())){
      DLOG(ERROR) << "cannot encoder list size.";
      return false;
    }
    for(auto& it : list){
      E encoder(it, flags);
      if(!encoder.Encode(buff)){
        DLOG(ERROR) << "cannot encoder list item: " << it;
        return false;
      }
    }
    return true;
  }

  bool Transaction::Encoder::Encode(const BufferPtr& buff) const{
    if(ShouldEncodeVersion()){
      if(!buff->PutVersion(codec::GetCurrentVersion())){
        CANNOT_ENCODE_CODEC_VERSION(FATAL);
        return false;
      }
      DLOG(INFO) << "encoded version: " << codec::GetCurrentVersion();
    }

    const Timestamp& timestamp = value().timestamp();
    if(!buff->PutTimestamp(timestamp)){
      DLOG(ERROR) << "cannot encoder transaction timestamp: " << FormatTimestampReadable(timestamp);
      return false;
    }
    DLOG(INFO) << "encoded  transaction timestamp: " << FormatTimestampReadable(timestamp);

    const InputList& inputs = value().inputs();
    if(!EncodeList<Input, Input::Encoder>(buff, inputs)){
      DLOG(ERROR) << "cannot encoder transaction inputs: " << inputs.size();
      return false;
    }
    DLOG(INFO) << "encoded transaction inputs: " << inputs.size();

    const OutputList& outputs = value().outputs();
    if(!EncodeList<Output, Output::Encoder>(buff, outputs)){
      DLOG(ERROR) << "cannot encoder transaction outputs: " << outputs.size();
      return false;
    }
    DLOG(INFO) << "encoded transaction outputs: " << outputs.size();
    return true;
  }

  template<class V, class D>
  static inline bool
  DecodeList(const BufferPtr& buff, std::vector<V>& list, const codec::DecoderHints& hints=codec::kDefaultDecoderHints){
    D decoder(hints);
    return buff->GetList<V, D>(list, hints);
  }

  bool Transaction::Decoder::Decode(const BufferPtr& buff, Transaction& result) const{
    CHECK_CODEC_VERSION(FATAL, buff);

    Timestamp timestamp = buff->GetTimestamp();
    DLOG(INFO) << "decoded IndexedTransaction timestamp: " << FormatTimestampReadable(timestamp);

    InputList inputs;
    if(!DecodeList<Input, Input::Decoder>(buff, inputs)){
      CANNOT_DECODE_VALUE(FATAL, inputs, InputList);
      return false;
    }
    DLOG(INFO) << "decoded inputs (InputList), " << inputs.size() << " items.";

    OutputList outputs;
    if(!DecodeList<Output, Output::Decoder>(buff, outputs)){
      CANNOT_DECODE_VALUE(FATAL, outputs, OutputList);
      return false;
    }
    DLOG(INFO) << "decoded outputs (OutputList), " << outputs.size() << " items.";

    result = Transaction(timestamp, inputs, outputs);
    return true;
  }
}*/
