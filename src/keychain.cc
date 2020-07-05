#include <glog/logging.h>
#include "common.h"
#include "keychain.h"
#include "crash_report.h"

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

#ifdef TOKEN_DEBUG
        LOG(INFO) << "generated private key: " << filename;
#endif//TOKEN_DEBUG
    }

    void EncodePublicKey(CryptoPP::RSA::PublicKey* key, const std::string& filename){
        CryptoPP::ByteQueue bytes;
        key->DEREncodePublicKey(bytes);
        Encode(bytes, filename);

#ifdef TOKEN_DEBUG
        LOG(INFO) << "generated public key: " << filename;
#endif//TOKEN_DEBUG
    }

#define PRIVATE_KEYFILE (TOKEN_BLOCKCHAIN_HOME + "/chain")
#define PUBLIC_KEYFILE (TOKEN_BLOCKCHAIN_HOME + "/chain.pub")

    void Keychain::Initialize(){
        if(!FileExists(PRIVATE_KEYFILE)){
            if(!FileExists(PUBLIC_KEYFILE)){
                if(FileExists((TOKEN_BLOCKCHAIN_HOME + "/state"))){
                    std::stringstream ss;
                    ss << "Failed to load block chain keys:" << std::endl;
                    ss << "  - Private Key: " << PRIVATE_KEYFILE << " - Missing" << std::endl;
                    ss << "  - Public Key: " << PUBLIC_KEYFILE << " - Missing" << std::endl;
                    ss << "Please recover the keys before running this node" << std::endl;
                    ss << "The node will require it's original keys, or it will need to be reinitialized before running";
                    CrashReport::GenerateAndExit(ss);
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
                    return;
                } catch(CryptoPP::Exception& ex){
                    std::stringstream ss;
                    ss << "An exception occurred when generating the block chain keys: " << std::endl;
                    ss << "\t" << ex.what();
                    CrashReport::GenerateAndExit(ss);
                }
            }

            std::stringstream ss;
            ss << "Failed to load block chain keys:" << std::endl;
            ss << "  - Private Key: " << PRIVATE_KEYFILE << " - Missing" << std::endl;
            ss << "  - Public Key: " << PUBLIC_KEYFILE << " - Found" << std::endl;
            CrashReport::GenerateAndExit(ss);
        }

        CryptoPP::RSA::PrivateKey privateKey;
        CryptoPP::RSA::PublicKey publicKey;
        LoadKeys(&privateKey, &publicKey);
    }

    void Keychain::LoadKeys(CryptoPP::RSA::PrivateKey* privKey, CryptoPP::RSA::PublicKey* pubKey){
        try{
            CryptoPP::AutoSeededRandomPool rng;

            // 1. Load + Validate Private Key
            std::string privateKeyFilename = (TOKEN_BLOCKCHAIN_HOME + "/chain");
            if(!FileExists(privateKeyFilename)){
                std::stringstream ss;
                ss << "Failed to load block chain keys:" << std::endl;
                ss << "  - Private Key: " << PRIVATE_KEYFILE << " - Missing" << std::endl;
                ss << "  - Public Key: " << PUBLIC_KEYFILE << " - Found" << std::endl;
                CrashReport::GenerateAndExit(ss);
            }
            DecodePrivateKey(privKey, privateKeyFilename);
            if(!privKey->Validate(rng, 3)){
                std::stringstream ss;
                ss << "Failed to load block chain keys:" << std::endl;
                ss << "  - Private Key: " << PRIVATE_KEYFILE << " - Invalid" << std::endl;
                ss << "  - Public Key: " << PUBLIC_KEYFILE << " - Found" << std::endl;
                CrashReport::GenerateAndExit(ss);
            }

            // 2. Load + Validate Public Key
            std::string publicKeyFilename = (TOKEN_BLOCKCHAIN_HOME + "/chain.pub");
            if(!FileExists(publicKeyFilename)){
                std::stringstream ss;
                ss << "Failed to load block chain keys:" << std::endl;
                ss << "  - Private Key: " << PRIVATE_KEYFILE << " - Found" << std::endl;
                ss << "  - Public Key: " << PUBLIC_KEYFILE << " - Missing" << std::endl;
                CrashReport::GenerateAndExit(ss);
            }
            DecodePublicKey(pubKey, publicKeyFilename);
            if(!pubKey->Validate(rng, 3)){
                std::stringstream ss;
                ss << "Failed to load block chain keys:" << std::endl;
                ss << "  - Private Key: " << PRIVATE_KEYFILE << " - Valid" << std::endl;
                ss << "  - Public Key: " << PUBLIC_KEYFILE << " - Invalid" << std::endl;
                CrashReport::GenerateAndExit(ss);
            }

            // 3. Consistency Check
            if((privKey->GetModulus() != pubKey->GetModulus()) || (privKey->GetPublicExponent() != pubKey->GetPublicExponent())) CrashReport::GenerateAndExit("Keys didn't pass round-trip consistency check");

#ifdef TOKEN_DEBUG
            LOG(INFO) << "initialized block chain keychain";
#endif//TOKEN_DEBUG
        } catch(CryptoPP::Exception& ex){
            std::stringstream ss;
            ss << "Failed to load block chain keys: " << ex.what();
            CrashReport::GenerateAndExit(ss);
        }
    }
}