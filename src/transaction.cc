#include "pool.h"
#include "keychain.h"
#include "blockchain.h"
#include "transaction.h"
#include "server/server.h"
#include "unclaimed_transaction.h"

namespace Token{
  TransactionPtr Transaction::NewInstance(const int64_t& index,
    const InputList& inputs,
    const OutputList& outputs,
    const Timestamp& timestamp){
    return std::make_shared<Transaction>(timestamp, index, inputs, outputs);
  }

  TransactionPtr Transaction::FromBytes(const BufferPtr& buff){
    Timestamp timestamp = FromUnixTimestamp(buff->GetLong());
    int64_t index = buff->GetLong();

    InputList inputs;

    int64_t idx;
    int64_t num_inputs = buff->GetLong();
    for(idx = 0; idx < num_inputs; idx++)
      inputs.push_back(Input(buff));

    OutputList outputs;
    int64_t num_outputs = buff->GetLong();
    for(idx = 0; idx < num_outputs; idx++)
      outputs.push_back(Output(buff));
    return std::make_shared<Transaction>(timestamp, index, inputs, outputs);
  }

  TransactionPtr Transaction::NewInstance(BinaryFileReader* reader){
    Timestamp timestamp = FromUnixTimestamp(reader->ReadLong());
    int64_t index = reader->ReadLong();

    int64_t idx;
    InputList inputs;
    int64_t num_inputs = reader->ReadLong();
    for(idx = 0; idx < num_inputs; idx++)
      inputs.push_back(Input(reader));

    OutputList outputs;
    int64_t num_outputs = reader->ReadLong();
    for(idx = 0; idx < num_outputs; idx++)
      outputs.push_back(Output(reader));
    return std::make_shared<Transaction>(timestamp, index, inputs, outputs);
  }

  bool Transaction::VisitInputs(TransactionInputVisitor* vis) const{
    for(auto& it : inputs_)
      if(!vis->Visit(it)){
        return false;
      }
    return true;
  }

  bool Transaction::VisitOutputs(TransactionOutputVisitor* vis) const{
    for(auto& it : outputs_)
      if(!vis->Visit(it)){
        return false;
      }
    return true;
  }

  bool Transaction::Sign(){
    CryptoPP::SecByteBlock bytes;
    /*
      TODO:
        if(!Encode(bytes)){
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
}