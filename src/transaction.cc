#include "transaction.h"
#include "keychain.h"
#include "block_chain.h"
#include "crash_report.h"
#include "server.h"
#include "unclaimed_transaction_pool.h"

namespace Token{
//######################################################################################################################
//                                          Input
//######################################################################################################################
    std::string Input::ToString() const{
        std::stringstream stream;
        stream << "Input(" << GetTransactionHash() << "[" << GetOutputIndex() << "]" << ")";
        return stream.str();
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
//######################################################################################################################
//                                          Transaction
//######################################################################################################################
    Handle<Transaction> Transaction::NewInstance(uint32_t index, Input** inputs, size_t num_inputs, Output** outputs, size_t num_outputs, uint32_t timestamp){
        return new Transaction(timestamp, index, inputs, num_inputs, outputs, num_outputs);
    }

    Handle<Transaction> Transaction::NewInstance(const RawTransaction& raw){
        size_t num_inputs = raw.inputs_size();
        Input* inputs[num_inputs];
        for(size_t idx = 0; idx < num_inputs; idx++){
            inputs[idx] = Input::NewInstance(raw.inputs(idx));
        }

        size_t num_outputs = raw.outputs_size();
        Output* outputs[num_outputs];
        for(size_t idx = 0; idx < num_outputs; idx++){
            outputs[idx] = Output::NewInstance(raw.outputs(idx));
        }

        return new Transaction(raw.timestamp(), raw.index(), inputs, num_inputs, outputs, num_outputs);
    }

    Handle<Transaction> Transaction::NewInstance(std::fstream& fd){
        RawTransaction raw;
        if(!raw.ParseFromIstream(&fd)) return nullptr;
        return NewInstance(raw);
    }

    std::string Transaction::ToString() const{
        std::stringstream stream;
        stream << "Transaction(" << GetSHA256Hash() << ")";
        return stream.str();
    }

    bool Transaction::WriteToMessage(RawTransaction& raw) const{
        raw.set_timestamp(timestamp_);
        raw.set_index(index_);
        raw.set_signature(signature_);
        for(uint32_t idx = 0; idx < GetNumberOfInputs(); idx++){
            RawInput* raw_in = raw.add_inputs();
            Handle<Input> it = GetInput(idx);
            raw_in->set_index(it->GetOutputIndex());
            raw_in->set_previous_hash(HexString(it->GetTransactionHash()));
            raw_in->set_user(it->GetUser());
        }
        for(uint32_t idx = 0; idx < GetNumberOfOutputs(); idx++){
            RawOutput* raw_out = raw.add_outputs();
            Handle<Output> it = GetOutput(idx);
            raw_out->set_user(it->GetUser());
            raw_out->set_token(it->GetToken());
        }
        return true;
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
        if(!WriteToBytes(bytes)){
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
}