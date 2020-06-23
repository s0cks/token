#include <glog/logging.h>
#include "common.h"
#include "keychain.h"

namespace Token{
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
    }

    void EncodePublicKey(CryptoPP::RSA::PublicKey* key, const std::string& filename){
        CryptoPP::ByteQueue bytes;
        key->DEREncodePublicKey(bytes);
        Encode(bytes, filename);
    }

#define PRIVATE_KEYFILE (TOKEN_BLOCKCHAIN_HOME + "/chain")
#define PUBLIC_KEYFILE (TOKEN_BLOCKCHAIN_HOME + "/chain.pub")

    bool Keychain::Initialize(){
        if(!FileExists(PRIVATE_KEYFILE)){
            LOG(WARNING) << "cannot find private key, checking for public key";
            if(!FileExists(PUBLIC_KEYFILE)){
                LOG(WARNING) << "cannot find public key, checking for chain state";
                if(FileExists((TOKEN_BLOCKCHAIN_HOME + "/state"))){
                    LOG(WARNING) << "chain state found";
                    LOG(WARNING) << "please recover your keys for this node";
                    LOG(WARNING) << "node will need it's original keys - or will need to be reinitialized";
                    return false;
                }

                LOG(INFO) << "no keys or chain state found, generating new keys";
                try{
                    CryptoPP::AutoSeededRandomPool rng;

                    LOG(INFO) << "generating private key";
                    CryptoPP::RSA::PrivateKey privKey;
                    privKey.GenerateRandomWithKeySize(rng, Keychain::kKeypairSize);

                    LOG(INFO) << "generating public key";
                    CryptoPP::RSA::PublicKey pubKey(privKey);

                    LOG(INFO) << "encoding keys";
                    EncodePrivateKey(&privKey, PRIVATE_KEYFILE);
                    EncodePublicKey(&pubKey, PUBLIC_KEYFILE);
                    LOG(INFO) << "encoded keys";
                    return true;
                } catch(CryptoPP::Exception& ex){
                    LOG(ERROR) << "error occurred generating keys: " << ex.GetWhat();
                    return false;
                }
            }

            LOG(WARNING) << "found a public key but not a private key, key is invalid";
            return false;
        }

        LOG(INFO) << "keys are found, testing validity";
        CryptoPP::RSA::PrivateKey privateKey;
        CryptoPP::RSA::PublicKey publicKey;
        if(!LoadKeys(&privateKey, &publicKey)){
            LOG(WARNING) << "keys were not loaded";
            return false;
        }
        return true;
    }

    bool Keychain::LoadKeys(CryptoPP::RSA::PrivateKey* privKey, CryptoPP::RSA::PublicKey* pubKey){
        try{
            std::string privateKeyFilename = (TOKEN_BLOCKCHAIN_HOME + "/chain");
            if(!FileExists(privateKeyFilename)){
                LOG(ERROR) << "cannot find private key '" << privateKeyFilename << "'";
                return false;
            }
            DecodePrivateKey(privKey, privateKeyFilename);
            LOG(INFO) << "done loading private key";

            std::string publicKeyFilename = (TOKEN_BLOCKCHAIN_HOME + "/chain.pub");
            if(!FileExists(publicKeyFilename)){
                LOG(ERROR) << "cannot find public key '" << publicKeyFilename << "'";
                return false;
            }
            DecodePublicKey(pubKey, publicKeyFilename);
            LOG(INFO) << "done loading public key";

            LOG(INFO) << "validating keys";
            CryptoPP::AutoSeededRandomPool rng;
            if(!privKey->Validate(rng, 3)){
                LOG(ERROR) << "private key is not valid";
                return false;
            }

            if(!pubKey->Validate(rng, 3)){
                LOG(ERROR) << "public key is not valid";
                return false;
            }

            LOG(INFO) << "checking for consistency";
            if((privKey->GetModulus() != pubKey->GetModulus()) || (privKey->GetPublicExponent() != pubKey->GetPublicExponent())){
                LOG(ERROR) << "keys did not pass consistency (round-trip) check";
                return false;
            }

            LOG(INFO) << "done loading keys";
            return true;
        } catch(CryptoPP::Exception& ex){
            return false;
        }
    }
}