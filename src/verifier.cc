#include <glog/logging.h>
#include <cryptopp/pssr.h>
#include "key.h"
#include "verifier.h"

namespace Token{
    bool TransactionVerifier::VerifySignature() {
        CryptoPP::RSA::PrivateKey privateKey;
        CryptoPP::RSA::PublicKey publicKey;
        if (!TokenKeychain::LoadKeys(&privateKey, &publicKey)) {
            LOG(ERROR) << "couldn't load keys";
            return false;
        }

        CryptoPP::RSASS<CryptoPP::PSS, CryptoPP::SHA256>::Signer signer;

        std::string sigstr = GetTransaction()->GetSignature();
        CryptoPP::SecByteBlock sigData(signer.MaxSignatureLength());
        CryptoPP::StringSource source(sigstr, true, new CryptoPP::HexDecoder(
                new CryptoPP::ArraySink(sigData.data(), sigData.size())));

        CryptoPP::RSASS<CryptoPP::PSS, CryptoPP::SHA256>::Verifier verifier(publicKey);

        uint64_t size = GetTransaction()->GetByteSize();
        uint8_t *bytes;
        if (!GetTransaction()->GetBytes(&bytes, size)) {
            LOG(ERROR) << "couldn't get transaction bytes";
            return false;
        }

        return verifier.VerifyMessage(bytes, size, sigData, sigData.size());
    }
}