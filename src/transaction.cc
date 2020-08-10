#include "transaction.h"
#include "keychain.h"
#include "block_chain.h"
#include "server.h"
#include "unclaimed_transaction_pool.h"

#include "bytes.h"

namespace Token{
//######################################################################################################################
//                                          Input
//######################################################################################################################
    Handle<Input> Input::NewInstance(ByteBuffer* bytes){
        uint256_t hash = bytes->GetHash();
        uint32_t index = bytes->GetInt();
        std::string user = bytes->GetString();
        return new Input(hash, index, user);
    }

    size_t Input::GetBufferSize() const{
        size_t size = 0;
        size += uint256_t::kSize; // hash_
        size += sizeof(uint32_t); // index_
        size += sizeof(uint32_t); // length(user_)
        size += user_.length();
        return size;
    }

    bool Input::Encode(ByteBuffer* bytes) const{
        bytes->PutHash(hash_);
        bytes->PutInt(index_);
        bytes->PutString(user_);
        return true;
    }

    std::string Input::ToString() const{
        std::stringstream stream;
        stream << "Input(" << GetTransactionHash() << "[" << GetOutputIndex() << "]" << ", " << GetUser() << ")";
        return stream.str();
    }

    UnclaimedTransaction* Input::GetUnclaimedTransaction() const{
        return UnclaimedTransactionPool::GetUnclaimedTransaction(hash_, index_);
    }

//######################################################################################################################
//                                          Output
//######################################################################################################################
    Handle<Output> Output::NewInstance(ByteBuffer* bytes){
        std::string user = bytes->GetString();
        std::string token = bytes->GetString();
        return new Output(user, token);
    }

    size_t Output::GetBufferSize() const{
        size_t size = 0;
        size += sizeof(uint32_t); // length(user_)
        size += user_.length();
        size += sizeof(uint32_t); // length(token_)
        size += token_.length();
        return size;
    }

    bool Output::Encode(ByteBuffer* bytes) const{
        bytes->PutString(user_);
        bytes->PutString(token_);
        return true;
    }

    std::string Output::ToString() const{
        std::stringstream stream;
        stream << "Output(" << GetUser() << "; " << GetToken() << ")";
        return stream.str();
    }
//######################################################################################################################
//                                          Transaction
//######################################################################################################################
    Handle<Transaction> Transaction::NewInstance(ByteBuffer* bytes){
        uint64_t timestamp = bytes->GetLong();
        uint32_t index = bytes->GetInt();
        uint32_t num_inputs = bytes->GetInt();
        Input* inputs[num_inputs];
        for(uint32_t idx = 0; idx < num_inputs; idx++)
            inputs[idx] = Input::NewInstance(bytes);
        uint32_t num_outputs = bytes->GetInt();
        Output* outputs[num_outputs];
        for(uint32_t idx = 0; idx < num_outputs; idx++)
            outputs[idx] = Output::NewInstance(bytes);
        return new Transaction(timestamp, index, inputs, num_inputs, outputs, num_outputs);
    }

    Handle<Transaction> Transaction::NewInstance(std::fstream& fd, size_t size){
        ByteBuffer bytes(size);
        fd.read((char*)bytes.data(), size);
        return NewInstance(&bytes);
    }

    size_t Transaction::GetBufferSize() const{
        size_t size = 0;
        size += sizeof(uint64_t); // timestamp_
        size += sizeof(uint32_t); // index_
        size += sizeof(uint32_t); // num_inputs_
        for(uint32_t idx = 0; idx < GetNumberOfInputs(); idx++)
            size += inputs_[idx]->GetBufferSize(); // inputs_[idx]
        size += sizeof(uint32_t); // num_outputs_
        for(uint32_t idx = 0; idx < GetNumberOfOutputs(); idx++)
            size += outputs_[idx]->GetBufferSize(); // outputs_[idx]
        return size;
    }

    bool Transaction::Encode(ByteBuffer* bytes) const{
        bytes->PutLong(timestamp_);
        bytes->PutInt(index_);
        bytes->PutInt(num_inputs_);
        uint32_t idx;
        for(idx = 0; idx < num_inputs_; idx++){
            Input* input = inputs_[idx];
            if(!input->Encode(bytes)) return false;
        }
        bytes->PutInt(num_outputs_);
        for(idx = 0; idx < num_outputs_; idx++){
            Output* output = outputs_[idx];
            if(!output->Encode(bytes)) return false;
        }
        return true;
    }

    std::string Transaction::ToString() const{
        std::stringstream stream;
        stream << "Transaction(" << GetHash() << ")";
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
            LOG(INFO) << "signing transaction: " << HexString(GetHash());
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
}