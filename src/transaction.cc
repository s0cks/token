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
        UserID user(bytes);
        return new Input(hash, index, user);
    }

    size_t Input::GetBufferSize() const{
        size_t size = 0;
        size += uint256_t::kSize; // hash_
        size += sizeof(uint32_t); // index_
        size += UserID::kSize;
        return size;
    }

    bool Input::Encode(ByteBuffer* bytes) const{
        bytes->PutHash(hash_);
        bytes->PutInt(index_);
        user_.Encode(bytes);
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
        UserID user(bytes);
        TokenID token(bytes);
        return new Output(user, token);
    }

    size_t Output::GetBufferSize() const{
        size_t size = 0;
        size += UserID::kSize;
        size += TokenID::kSize;
        return size;
    }

    bool Output::Encode(ByteBuffer* bytes) const{
        user_.Encode(bytes);
        token_.Encode(bytes);
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

        // parse inputs
        uint32_t num_inputs = bytes->GetInt();
        Input* inputs[num_inputs];
        for(uint32_t idx = 0; idx < num_inputs; idx++)
            inputs[idx] = Input::NewInstance(bytes);

        // parse outputs
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
            size += GetInput(idx)->GetBufferSize(); // inputs_[idx]
        size += sizeof(uint32_t); // num_outputs_
        for(uint32_t idx = 0; idx < GetNumberOfOutputs(); idx++)
            size += GetOutput(idx)->GetBufferSize(); // outputs_[idx]
        return size;
    }

    bool Transaction::Encode(ByteBuffer* bytes) const{
        bytes->PutLong(timestamp_);
        bytes->PutInt(index_);

        size_t num_inputs = GetNumberOfInputs();
        bytes->PutInt(num_inputs);
        uint32_t idx;
        for(idx = 0; idx < num_inputs; idx++){
            Handle<Input> input = GetInput(idx);
            if(!input->Encode(bytes))
                return false;
        }

        size_t num_outputs = GetNumberOfOutputs();
        bytes->PutInt(num_outputs);
        for(idx = 0; idx < num_outputs; idx++){
            Handle<Output> output = GetOutput(idx);
            if(!output->Encode(bytes))
                return false;
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