#include "transaction.h"
#include "keychain.h"
#include "block_chain.h"
#include "crash_report.h"
#include "server.h"
#include "unclaimed_transaction_pool.h"

#include "bytes.h"

namespace Token{
//######################################################################################################################
//                                          Input
//######################################################################################################################
    Handle<Input> Input::NewInstance(uint8_t* bytes){
        size_t offset = 0;
        uint256_t hash = DecodeHash(&bytes[offset]);
        offset += uint256_t::kSize;

        uint32_t index = DecodeInt(&bytes[offset]);
        offset += 4;

        uint32_t user_length = DecodeInt(&bytes[offset]);
        offset += 4;

        std::string user = DecodeString(&bytes[offset], user_length);
        return NewInstance(hash, index, user);
    }

    std::string Input::ToString() const{
        std::stringstream stream;
        stream << "Input(" << GetTransactionHash() << "[" << GetOutputIndex() << "]" << ", " << GetUser() << ")";
        return stream.str();
    }

    size_t Input::GetBufferSize() const{
        size_t size = 0;
        size += uint256_t::kSize;
        size += 4;
        size += 4 + user_.length();
        return size;
    }

    bool Input::Encode(uint8_t* bytes) const{
        size_t offset = 0;
        EncodeHash(&bytes[offset], hash_);
        offset += uint256_t::kSize;
        EncodeInt(&bytes[offset], index_);
        offset += 4;
        EncodeString(&bytes[offset], user_);
        return true;
    }

    UnclaimedTransaction* Input::GetUnclaimedTransaction() const{
        return UnclaimedTransactionPool::GetUnclaimedTransaction(hash_, index_);
    }

//######################################################################################################################
//                                          Output
//######################################################################################################################
    std::string Output::ToString() const{
        std::stringstream stream;
        stream << "Output(" << GetUser() << "; " << GetToken() << ")";
        return stream.str();
    }

    size_t Output::GetBufferSize() const{
        size_t size = 0;
        size += 4 + user_.length();
        size += 4 + token_.length();
        return size;
    }

    bool Output::Encode(uint8_t* bytes) const{
        EncodeString(&bytes[GetUserOffset()], user_);
        EncodeString(&bytes[GetTokenOffset()], token_);
        return true;
    }

    Handle<Output> Output::NewInstance(uint8_t* bytes){
        size_t offset = 0;

        uint32_t user_length = DecodeInt(&bytes[offset]);
        offset += 4;
        std::string user = DecodeString(&bytes[offset], user_length);
        offset += user.length();

        uint32_t token_length = DecodeInt(&bytes[offset]);
        offset += 4;
        std::string token = DecodeString(&bytes[offset], token_length);
        return new Output(user, token);
    }
//######################################################################################################################
//                                          Transaction
//######################################################################################################################
    Handle<Transaction> Transaction::NewInstance(uint32_t index, Input** inputs, size_t num_inputs, Output** outputs, size_t num_outputs, uint32_t timestamp){
        return new Transaction(timestamp, index, inputs, num_inputs, outputs, num_outputs);
    }

    Handle<Transaction> Transaction::NewInstance(uint8_t* bytes){
        size_t offset = 0;

        uint32_t timestamp = DecodeInt(&bytes[offset]);
        offset += 4;

        uint32_t index = DecodeInt(&bytes[offset]);
        offset += 4;

        uint32_t num_inputs = DecodeInt(&bytes[offset]);
        offset += 4;

        Input* inputs[num_inputs];
        for(uint32_t idx = 0; idx < num_inputs; idx++){
            Handle<Input> input = inputs[idx] = Input::NewInstance(&bytes[offset]);
            offset += input->GetBufferSize();
        }

        uint32_t num_outputs = DecodeInt(&bytes[offset]);
        offset += 4;

        Output* outputs[num_outputs];
        for(uint32_t idx = 0; idx < num_outputs; idx++){
            Handle<Output> output = outputs[idx] = Output::NewInstance(&bytes[offset]);
            offset += output->GetBufferSize();
        }
        return new Transaction(timestamp, index, inputs, num_inputs, outputs, num_outputs);
    }

    Handle<Transaction> Transaction::NewInstance(std::fstream& fd){
        //TODO: implement
        return nullptr;
    }

    std::string Transaction::ToString() const{
        std::stringstream stream;
        stream << "Transaction(" << GetSHA256Hash() << ")";
        return stream.str();
    }

    bool Transaction::Accept(Token::TransactionVisitor* vis){
        if(!vis->VisitStart()) return false;
        // visit the inputs
        {
            if(!vis->VisitInputsStart()) return false;
            for(uint64_t idx = 0; idx < GetNumberOfInputs(); idx++){
                Handle<Input> it = GetInput(idx);
                if(!vis->VisitInput(it)) return false;
            }
            if(!vis->VisitInputsEnd()) return false;
        }
        // visit the outputs
        {
            if(!vis->VisitOutputsStart()) return false;
            for(uint64_t idx = 0; idx < GetNumberOfOutputs(); idx++){
                Handle<Output> it = GetOutput(idx);
                if(!vis->VisitOutput(it)) return false;
            }
            if(!vis->VisitOutputsEnd()) return false;
        }
        return vis->VisitEnd();
    }

    bool Transaction::Sign(){
        CryptoPP::SecByteBlock bytes;
        if(!Encode(bytes)){
            LOG(WARNING) << "couldn't serialize transaction to bytes";
            return false;
        }

        CryptoPP::RSA::PrivateKey privateKey;
        CryptoPP::RSA::PublicKey publicKey;
        Keychain::LoadKeys(&privateKey, &publicKey);

        try{
            LOG(INFO) << "signing transaction: " << HexString(GetSHA256Hash());
            CryptoPP::RSASS<CryptoPP::PSS, CryptoPP::SHA256>::Signer signer(privateKey);
            CryptoPP::AutoSeededRandomPool rng;
            CryptoPP::SecByteBlock sigData(signer.MaxSignatureLength());

            size_t length = signer.SignMessage(rng, bytes.data(), bytes.size(), sigData);
            sigData.resize(length);

            std::string signature;
            CryptoPP::ArraySource source(sigData.data(), sigData.size(), true, new CryptoPP::HexEncoder(new CryptoPP::StringSink(signature)));

            LOG(INFO) << "signature: " << signature;
            signature_ = signature;
            return true;
        } catch(CryptoPP::Exception& ex){
            LOG(ERROR) << "error occurred signing transaction: " << ex.GetWhat();
            return false;
        }
    }

    uint256_t Transaction::GetSHA256Hash() const{
        try{
            size_t size = GetBufferSize();
            CryptoPP::SHA256 func;
            CryptoPP::SecByteBlock bytes(size);
            if(!Encode(bytes)){
                LOG(WARNING) << "couldn't encode transaction to bytes";
                return uint256_t();
            }

            CryptoPP::SecByteBlock hash(CryptoPP::SHA256::DIGESTSIZE);
            CryptoPP::ArraySource source(bytes.data(), bytes.size(), true, new CryptoPP::HashFilter(func, new CryptoPP::ArraySink(hash.data(), hash.size())));
            return uint256_t(hash.data());
        } catch(CryptoPP::Exception& exc){
            LOG(WARNING) << "exception occurred getting block hash: " << exc.what();
        }
    }

    size_t Transaction::GetBufferSize() const{
        size_t size = 0;
        size += 4; // timestamp_
        size += 4; // index_
        size += 4; // num_inputs
        for(uint32_t idx = 0; idx < GetNumberOfInputs(); idx++)
            size += inputs_[idx]->GetBufferSize();
        size += 4; // num_outputs
        for(uint32_t idx = 0; idx < GetNumberOfOutputs(); idx++)
            size += outputs_[idx]->GetBufferSize();
        size += (4 + signature_.length());
        return size;
    }

    bool Transaction::Encode(uint8_t* bytes) const{
        size_t offset = 0;
        EncodeInt(&bytes[offset], timestamp_);
        offset += 4;

        EncodeInt(&bytes[offset], index_);
        offset += 4;

        EncodeInt(&bytes[offset], num_inputs_);
        offset += 4;
        for(uint32_t idx = 0; idx < num_inputs_; idx++){
            Input* input = inputs_[idx];
            if(!input->Encode(&bytes[offset])) return false;
            offset += input->GetBufferSize();
        }

        EncodeInt(&bytes[offset], num_outputs_);
        offset += 4;
        for(uint32_t idx = 0; idx < num_outputs_; idx++){
            Output* output = outputs_[idx];
            if(!output->Encode(&bytes[offset])) return false;
            offset += output->GetBufferSize();
        }

        EncodeString(&bytes[offset], signature_);
        return true;
    }
}