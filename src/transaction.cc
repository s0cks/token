#include "transaction.h"
#include "keychain.h"
#include "block_chain.h"
#include "server.h"
#include "unclaimed_transaction_pool.h"

#include "byte_buffer.h"

namespace Token{
//######################################################################################################################
//                                          Input
//######################################################################################################################
    Handle<Input> Input::NewInstance(ByteBuffer* bytes){
        Hash hash = bytes->GetHash();
        uint32_t index = bytes->GetInt();
        User user(bytes);
        return new Input(hash, index, user);
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
        User user(bytes);
        Product product(bytes);
        return new Output(user, product);
    }

    std::string Output::ToString() const{
        std::stringstream stream;
        stream << "Output(" << GetUser() << "; " << GetProduct() << ")";
        return stream.str();
    }
//######################################################################################################################
//                                          Transaction
//######################################################################################################################
    Handle<Transaction> Transaction::NewInstance(ByteBuffer* bytes){
        Timestamp timestamp = bytes->GetLong();
        intptr_t index = bytes->GetLong();
        intptr_t num_inputs = bytes->GetLong();
        Input* inputs[num_inputs];
        for(uint32_t idx = 0; idx < num_inputs; idx++)
            inputs[idx] = Input::NewInstance(bytes);

        // parse outputs
        intptr_t num_outputs = bytes->GetLong();
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