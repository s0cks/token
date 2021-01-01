#include "transaction.h"
#include "keychain.h"
#include "blockchain.h"
#include "server.h"
#include "unclaimed_transaction.h"

namespace Token{
  UnclaimedTransactionPtr Input::GetUnclaimedTransaction() const{
    return UnclaimedTransactionPtr(nullptr);
    //TODO: fixme
//        return UnclaimedTransactionPool::GetUnclaimedTransaction(GetTransactionHash(), GetOutputIndex());
  }
//######################################################################################################################
//                                          Transaction
//######################################################################################################################
  const int64_t Transaction::kMaxNumberOfInputs = 40000;
  const int64_t Transaction::kMaxNumberOfOutputs = 40000;

  bool Transaction::VisitInputs(TransactionInputVisitor *vis) const{
    for(auto &it : inputs_)
      if(!vis->Visit(it))
        return false;
    return true;
  }

  bool Transaction::VisitOutputs(TransactionOutputVisitor *vis) const{
    for(auto &it : outputs_)
      if(!vis->Visit(it))
        return false;
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
    } catch(CryptoPP::Exception &ex){
      LOG(ERROR) << "error occurred signing transaction: " << ex.GetWhat();
      return false;
    }
  }
}