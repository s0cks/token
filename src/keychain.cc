#include <glog/logging.h>
#include "common.h"
#include "keychain.h"

namespace token{
  void Decode(CryptoPP::BufferedTransformation& bt, const std::string& filename){
    CryptoPP::FileSource source(filename.c_str(), true);
    source.TransferTo(bt);
    bt.MessageEnd();
  }

  void DecodePrivateKey(CryptoPP::RSA::PrivateKey* privKey, const std::string& filename){
    CryptoPP::ByteQueue bytes;
    Decode(bytes, filename);
    privKey->BERDecodePrivateKey(bytes, false, bytes.MaxRetrievable());
  }

  void DecodePublicKey(CryptoPP::RSA::PublicKey* key, const std::string& filename){
    CryptoPP::ByteQueue bytes;
    Decode(bytes, filename);
    key->BERDecodePublicKey(bytes, false, bytes.MaxRetrievable());
  }

  void Encode(CryptoPP::BufferedTransformation& bt, const std::string& filename){
    CryptoPP::FileSink sink(filename.c_str());
    bt.CopyTo(sink);
    sink.MessageEnd();
  }

  void EncodePrivateKey(CryptoPP::RSA::PrivateKey* key, const std::string& filename){
    CryptoPP::ByteQueue bytes;
    key->DEREncodePrivateKey(bytes);
    Encode(bytes, filename);
    LOG(INFO) << "generated private key: " << filename;
  }

  void EncodePublicKey(CryptoPP::RSA::PublicKey* key, const std::string& filename){
    CryptoPP::ByteQueue bytes;
    key->DEREncodePublicKey(bytes);
    Encode(bytes, filename);
    LOG(INFO) << "generated public key: " << filename;
  }

#define PRIVATE_KEYFILE (TOKEN_BLOCKCHAIN_HOME + "/chain")
#define PUBLIC_KEYFILE (TOKEN_BLOCKCHAIN_HOME + "/chain.pub")

  bool Keychain::Initialize(){
    if(!FileExists(PRIVATE_KEYFILE)){
      if(!FileExists(PUBLIC_KEYFILE)){
        if(FileExists((TOKEN_BLOCKCHAIN_HOME + "/data"))){
          LOG(WARNING) << "Failed to load block chain keys:" << std::endl;
          LOG(WARNING) << "  - Private Key: " << PRIVATE_KEYFILE << " - Missing" << std::endl;
          LOG(WARNING) << "  - Public Key: " << PUBLIC_KEYFILE << " - Missing" << std::endl;
          LOG(WARNING) << "Please recover the keys before running this node" << std::endl;
          LOG(WARNING)
            << "The node will require it's original keys, or it will need to be reinitialized before running";
          return false;
        }

        try{
          CryptoPP::AutoSeededRandomPool rng;

          // 1. Generate Private Key
          CryptoPP::RSA::PrivateKey privKey;
          privKey.GenerateRandomWithKeySize(rng, Keychain::kKeypairSize);

          // 2. Generate Public Key
          CryptoPP::RSA::PublicKey pubKey(privKey);

          // 3. Write Keys to File
          EncodePrivateKey(&privKey, PRIVATE_KEYFILE);
          EncodePublicKey(&pubKey, PUBLIC_KEYFILE);
        } catch(CryptoPP::Exception& ex){
          LOG(WARNING) << "An exception occurred when generating the block chain keys: " << std::endl;
          LOG(WARNING) << "\t" << ex.what();
        }
      } else{
        LOG(WARNING) << "Failed to load block chain keys:" << std::endl;
        LOG(WARNING) << "  - Private Key: " << PRIVATE_KEYFILE << " - Missing" << std::endl;
        LOG(WARNING) << "  - Public Key: " << PUBLIC_KEYFILE << " - Found" << std::endl;
        return false;
      }
    }

    CryptoPP::RSA::PrivateKey privateKey;
    CryptoPP::RSA::PublicKey publicKey;
    LoadKeys(&privateKey, &publicKey);
    return true;
  }

  bool Keychain::LoadKeys(CryptoPP::RSA::PrivateKey* privKey, CryptoPP::RSA::PublicKey* pubKey){
    try{
      CryptoPP::AutoSeededRandomPool rng;

      // 1. Load + Validate Private Key
      std::string privateKeyFilename = (TOKEN_BLOCKCHAIN_HOME + "/chain");
      if(!FileExists(privateKeyFilename)){
        LOG(WARNING) << "Failed to load block chain keys:" << std::endl;
        LOG(WARNING) << "  - Private Key: " << PRIVATE_KEYFILE << " - Missing" << std::endl;
        LOG(WARNING) << "  - Public Key: " << PUBLIC_KEYFILE << " - Found" << std::endl;
        return false;
      }

      DecodePrivateKey(privKey, privateKeyFilename);
      if(!privKey->Validate(rng, 3)){
        LOG(WARNING) << "Failed to load block chain keys:" << std::endl;
        LOG(WARNING) << "  - Private Key: " << PRIVATE_KEYFILE << " - Invalid" << std::endl;
        LOG(WARNING) << "  - Public Key: " << PUBLIC_KEYFILE << " - Found" << std::endl;
        return false;
      }

      // 2. Load + Validate Public Key
      std::string publicKeyFilename = (TOKEN_BLOCKCHAIN_HOME + "/chain.pub");
      if(!FileExists(publicKeyFilename)){
        LOG(WARNING) << "Failed to load block chain keys:" << std::endl;
        LOG(WARNING) << "  - Private Key: " << PRIVATE_KEYFILE << " - Found" << std::endl;
        LOG(WARNING) << "  - Public Key: " << PUBLIC_KEYFILE << " - Missing" << std::endl;
        return false;
      }

      DecodePublicKey(pubKey, publicKeyFilename);
      if(!pubKey->Validate(rng, 3)){
        LOG(WARNING) << "Failed to load block chain keys:" << std::endl;
        LOG(WARNING) << "  - Private Key: " << PRIVATE_KEYFILE << " - Valid" << std::endl;
        LOG(WARNING) << "  - Public Key: " << PUBLIC_KEYFILE << " - Invalid" << std::endl;
        return false;
      }

      // 3. Consistency Check
      if((privKey->GetModulus() != pubKey->GetModulus())
         || (privKey->GetPublicExponent() != pubKey->GetPublicExponent())){
        LOG(WARNING) << "Keys didn't pass round-trip consistency check";
        return false;
      }

      LOG(INFO) << "initialized block chain keychain";
      return true;
    } catch(CryptoPP::Exception& ex){
      LOG(WARNING) << "failed to load block chain keys: " << ex.what();
      return false;
    }
  }
}