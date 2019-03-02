#include "user.h"

namespace Token{
    static inline void
    Encode(const std::string& filename, CryptoPP::BufferedTransformation& bt){
        CryptoPP::FileSink file(filename.c_str());
        bt.CopyTo(file);
        file.MessageEnd();
    }

    static inline void
    EncodePrivateKey(const std::string& filename, const CryptoPP::RSA::PrivateKey& key){
        CryptoPP::ByteQueue queue;
        key.DEREncodePrivateKey(queue);
        Encode(filename, queue);
    }

    static inline void
    EncodePublicKey(const std::string& filename, const CryptoPP::RSA::PublicKey& key){
        CryptoPP::ByteQueue queue;
        key.DEREncodePublicKey(queue);
        Encode(filename, queue);
    }

    static inline void
    Decode(const std::string& filename, CryptoPP::BufferedTransformation& bt){
        CryptoPP::FileSource file(filename.c_str(), true);
        file.TransferTo(bt);
        bt.MessageEnd();
    }

    static inline void
    DecodePrivateKey(const std::string& filename, CryptoPP::RSA::PrivateKey& key){
        CryptoPP::ByteQueue queue;
        Decode(filename, queue);
        key.BERDecodePrivateKey(queue, false, queue.MaxRetrievable());
    }

    static inline void
    DecodePublicKey(const std::string& filename, CryptoPP::RSA::PublicKey& key){
        CryptoPP::ByteQueue queue;
        Decode(filename, queue);
        key.BERDecodePublicKey(queue, false, queue.MaxRetrievable());
    }

    static inline void
    GenerateKeyPair(CryptoPP::RSA::PublicKey& pubkey, CryptoPP::RSA::PrivateKey& privkey){
        CryptoPP::AutoSeededRandomPool rng;
        CryptoPP::InvertibleRSAFunction params;
        params.GenerateRandomWithKeySize(rng, User::KEY_LENGTH);
    }

    User::User(std::string userid):
        userid_(userid),
        private_key_(),
        public_key_(){
        GenerateKeyPair(public_key_, private_key_);
    }

    User::User(std::string userid, const std::string& pubkey, const std::string& privkey):
        userid_(userid),
        public_key_(),
        private_key_(){
        DecodePrivateKey(privkey, private_key_);
        DecodePublicKey(pubkey, public_key_);
    }

    void User::Save(const std::string &pubkey, const std::string &privkey){
        EncodePublicKey(pubkey, public_key_);
        EncodePrivateKey(privkey, private_key_);
    }
}