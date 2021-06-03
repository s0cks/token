#include "pool.h"
#include "keychain.h"
#include "blockchain.h"
#include "transaction.h"
#include "server.h"
#include "unclaimed_transaction.h"

namespace token{
  bool Transaction::Write(const BufferPtr& buff) const{
    return buff->PutTimestamp(timestamp_)
        && buff->PutVector(inputs_)
        && buff->PutVector(outputs_);
  }

  bool Transaction::Sign(){
    CryptoPP::SecByteBlock bytes;
    /*
      TODO:
        if(!WriteMessage(bytes)){
            LOG(WARNING) << "couldn't serialize transaction to bytes";
            return false;
        }
     */

    CryptoPP::RSA::PrivateKey privateKey;
    CryptoPP::RSA::PublicKey publicKey;
    Keychain::LoadKeys(&privateKey, &publicKey);

    try{
      LOG(INFO) << "signing transaction: " << GetHash();
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

  TransactionPtr Transaction::FromBytes(const BufferPtr& buff){
    Timestamp timestamp = buff->GetTimestamp();

    InputList inputs;
    if(!buff->GetList(inputs)){
      LOG(WARNING) << "cannot read inputs from buffer of size " << buff->GetWrittenBytes();
      return nullptr;
    }

    OutputList outputs;
    if(!buff->GetList(outputs)){
      LOG(WARNING) << "cannot read outputs from buffer of size " << buff->GetWrittenBytes();
      return nullptr;
    }
    return std::make_shared<Transaction>(timestamp, inputs, outputs);
  }
}