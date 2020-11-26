#include "transaction.h"
#include "keychain.h"
#include "block_chain.h"
#include "server.h"
#include "unclaimed_transaction_pool.h"

namespace Token{
//######################################################################################################################
//                                          Input
//######################################################################################################################
    Handle<Input> Input::NewInstance(const Handle<Buffer>& buff){
        Hash hash = buff->GetHash();
        int32_t index = buff->GetInt();
        User user = buff->GetUser();
        return new Input(hash, index, user);
    }

    bool Input::Write(const Handle<Buffer>& buff) const{
        buff->PutHash(hash_);
        buff->PutInt(index_);
        buff->PutUser(user_);
        return true;
    }

    std::string Input::ToString() const{
        std::stringstream stream;
        stream << "Input(" << GetTransactionHash() << "[" << GetOutputIndex() << "]" << ", " << GetUser() << ")";
        return stream.str();
    }

    UnclaimedTransaction* Input::GetUnclaimedTransaction() const{
        return UnclaimedTransactionPool::GetUnclaimedTransaction(GetTransactionHash(), GetOutputIndex());
    }

//######################################################################################################################
//                                          Output
//######################################################################################################################
    Handle<Output> Output::NewInstance(const Handle<Buffer>& buff){
        User user = buff->GetUser();
        Product product = buff->GetProduct();
        return new Output(user, product);
    }

    bool Output::Write(const Handle<Buffer>& buff) const{
        buff->PutUser(user_);
        buff->PutProduct(product_);
        return true;
    }

    std::string Output::ToString() const{
        std::stringstream stream;
        stream << "Output(" << GetUser() << "; " << GetProduct() << ")";
        return stream.str();
    }
//######################################################################################################################
//                                          Transaction
//######################################################################################################################
    Handle<Transaction> Transaction::NewInstance(std::fstream& fd, size_t size){
        Handle<Buffer> buff = Buffer::NewInstance(size);
        buff->ReadBytesFrom(fd, size);
        return NewInstance(buff);
    }

    Handle<Transaction> Transaction::NewInstance(const Handle<Buffer>& buff){
        Timestamp timestamp = buff->GetLong();
        intptr_t index = buff->GetLong();
        intptr_t num_inputs = buff->GetLong();
        Input* inputs[num_inputs];
        for(uint32_t idx = 0; idx < num_inputs; idx++)
            inputs[idx] = Input::NewInstance(buff);


        intptr_t num_outputs = buff->GetLong();
        Output* outputs[num_outputs];
        for(uint32_t idx = 0; idx < num_outputs; idx++){
            outputs[idx] = Output::NewInstance(buff);
        }
        return new Transaction(timestamp, index, inputs, num_inputs, outputs, num_outputs);
    }

    intptr_t Transaction::GetBufferSize() const{
        intptr_t size = 0;
        size += sizeof(Timestamp); // timestamp_
        size += sizeof(int64_t); // index_
        size += sizeof(int64_t); // num_inputs_
        size += (num_inputs_ * Input::kSize); // inputs_
        size += sizeof(int64_t); // num_outputs
        size += (num_outputs_ * Output::kSize); // outputs_
        return size;
    }

    bool Transaction::Write(const Handle<Buffer>& buff) const{
        buff->PutLong(timestamp_);
        buff->PutLong(index_);
        buff->PutLong(num_inputs_);
        for(intptr_t idx = 0; idx < num_inputs_; idx++){
            inputs_[idx]->Write(buff);
        }
        buff->PutLong(num_outputs_);
        for(intptr_t idx = 0; idx < num_outputs_; idx++){
            outputs_[idx]->Write(buff);
        }
        //TODO: serialize transaction signature
        return true;
    }

    std::string Transaction::ToString() const{
        std::stringstream stream;
        stream << "Transaction(#" << GetIndex() << "," << GetNumberOfInputs() << " Inputs, " << GetNumberOfOutputs() << " Outputs)";
        return stream.str();
    }

    bool Transaction::Accept(Token::TransactionVisitor* vis){
        if(!vis->VisitStart()) return false;
        // visit the inputs
        {
            if(!vis->VisitInputsStart()) return false;
            for(intptr_t idx = 0; idx < GetNumberOfInputs(); idx++){
                Handle<Input> it = GetInput(idx);
                if(!vis->VisitInput(it)) return false;
            }
            if(!vis->VisitInputsEnd()) return false;
        }
        // visit the outputs
        {
            if(!vis->VisitOutputsStart()) return false;
            for(intptr_t idx = 0; idx < GetNumberOfOutputs(); idx++){
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
            LOG(INFO) << "signing transaction: " << GetHash();
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