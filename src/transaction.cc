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
    Handle<Transaction> Transaction::NewInstance(uint32_t index, Transaction::InputList& inputs, Transaction::OutputList& outputs, uint32_t timestamp){
        return new Transaction(timestamp, index, inputs, outputs);
    }

    Handle<Transaction> Transaction::NewInstance(const RawTransaction& raw){
        Transaction::InputList inputs;
        for(auto& it : raw.inputs()){
            inputs.push_back(Input(it));
        }

        Transaction::OutputList outputs;
        for(auto& it : raw.outputs()){
            outputs.push_back(Output(it));
        }

        return new Transaction(raw.timestamp(), raw.index(), inputs, outputs);
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
        for(auto& it : inputs_){
            RawInput* raw_in = raw.add_inputs();
            raw_in->set_index(it.GetOutputIndex());
            raw_in->set_previous_hash(HexString(it.GetTransactionHash()));
            raw_in->set_user(it.GetUser());
        }
        for(auto& it : outputs_){
            RawOutput* raw_out = raw.add_outputs();
            raw_out->set_user(it.GetUser());
            raw_out->set_token(it.GetToken());
        }
        return true;
    }

    bool Transaction::Accept(Token::TransactionVisitor* vis){
        if(!vis->VisitStart()) return false;
        // visit the inputs
        {
            if(!vis->VisitInputsStart()) return false;
            for(uint64_t idx = 0; idx < GetNumberOfInputs(); idx++){
                Input input;
                if(!GetInput(idx, &input)) return false;
                if(!vis->VisitInput(&input)) return false;
            }
            if(!vis->VisitInputsEnd()) return false;
        }
        // visit the outputs
        {
            if(!vis->VisitOutputsStart()) return false;
            for(uint64_t idx = 0; idx < GetNumberOfOutputs(); idx++){
                Output output;
                if(!GetOutput(idx, &output)) return false;
                if(!vis->VisitOutput(&output)) return false;
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