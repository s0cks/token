#include <string>
#include <glog/logging.h>
#include <cryptopp/pssr.h>
#include "signer.h"
#include "key.h"

namespace Token{
    bool TransactionSigner::GetSignature(std::string* signature){
        uint64_t size = GetTransaction()->GetByteSize();
        uint8_t* bytes;
        if(!GetTransaction()->GetBytes(&bytes, size)){
            LOG(ERROR) << "couldn't get bytes for the transaction";

            free(bytes);
            return false;
        }

        CryptoPP::RSA::PrivateKey privateKey;
        CryptoPP::RSA::PublicKey publicKey;
        if(!TokenKeychain::LoadKeys(&privateKey, &publicKey)){
            LOG(WARNING) << "couldn't load chain keys";

            free(bytes);
            return false;
        }

        try{
            LOG(INFO) << "signing transaction: " << GetTransaction()->GetHash();
            CryptoPP::RSASS<CryptoPP::PSS, CryptoPP::SHA256>::Signer signer(privateKey);
            CryptoPP::AutoSeededRandomPool rng;

            CryptoPP::SecByteBlock sigData(signer.MaxSignatureLength());
            size_t length = signer.SignMessage(rng, bytes, size, sigData);
            sigData.resize(length);

            CryptoPP::ArraySource source(sigData.data(), sigData.size(), true, new CryptoPP::HexEncoder(new CryptoPP::StringSink(*signature)));

            free(bytes);
            return true;
        } catch(CryptoPP::Exception& ex){
            LOG(ERROR) << "error occurred signing transaction: " << ex.GetWhat();
            return false;
        }
    }

    bool TransactionSigner::Sign(){
        std::string signature;
        if(!GetSignature(&signature)){
            LOG(ERROR) << "couldn't generate signature";
            return false;
        }
        GetTransaction()->SetSignature(signature);
        return true;
    }
}
