#ifndef TOKEN_TRANSACTION_H
#define TOKEN_TRANSACTION_H

#include "common.h"
#include "object.h"
#include "uint256_t.h"
#include "unclaimed_transaction.h"

namespace Token{
    typedef Proto::BlockChain::Input RawInput;
    class Input : public Object{
        friend class Transaction;
    private:
        uint256_t hash_;
        uint32_t index_;
        std::string user_;

        Input(const uint256_t& tx_hash, uint32_t index, const std::string& user):
            hash_(tx_hash),
            user_(user),
            index_(index){}
        Input(const RawInput& raw):
            hash_(HashFromHexString(raw.previous_hash())),
            index_(raw.index()),
            user_(raw.user()){}
    public:
        ~Input(){}

        uint32_t GetOutputIndex() const{
            return index_;
        }

        uint256_t GetTransactionHash() const{
            return hash_;
        }

        std::string GetUser() const{
            return user_;
        }

        UnclaimedTransaction* GetUnclaimedTransaction() const;
        std::string ToString() const;

        static Handle<Input> NewInstance(const uint256_t& hash, uint32_t index, const std::string& user){
            return new Input(hash, index, user);
        }

        static Handle<Input> NewInstance(const RawInput& raw){
            return new Input(raw);
        }
    };

    typedef Proto::BlockChain::Output RawOutput;
    class Output : public Object{
        friend class Transaction;
    private:
        std::string user_;
        std::string token_;

        Output(const std::string& user, const std::string& token):
            user_(user),
            token_(token){}
        Output(const RawOutput& raw):
            user_(raw.user()),
            token_(raw.token()){}
    public:
        ~Output(){}

        std::string GetUser() const{
            return user_;
        }

        std::string GetToken() const{
            return token_;
        }

        std::string ToString() const;

        static Handle<Output> NewInstance(const std::string& user, const std::string& token){
            return new Output(user, token);
        }

        static Handle<Output> NewInstance(const RawOutput& raw){
            return new Output(raw);
        }
    };

    typedef Proto::BlockChain::Transaction RawTransaction;

    class TransactionVisitor;
    class Transaction : public BinaryObject<RawTransaction>{
        friend class Block;
        friend class TransactionMessage;
        friend class TransactionVerifier;
    private:
        uint32_t timestamp_;
        uint32_t index_;
        Input** inputs_;
        size_t num_inputs_;
        Output** outputs_;
        size_t num_outputs_;
        std::string signature_;

        Transaction(uint32_t timestamp, uint32_t index, Input** inputs, size_t num_inputs, Output** outputs, size_t num_outputs):
            BinaryObject(),
            timestamp_(timestamp),
            index_(index),
            inputs_(nullptr),
            num_inputs_(num_inputs),
            outputs_(nullptr),
            num_outputs_(num_outputs),
            signature_(){
            inputs_ = (Input**)malloc(sizeof(Input*)*num_inputs);
            memset(inputs_, 0, sizeof(Input*)*num_inputs);
            for(size_t idx = 0; idx < num_inputs; idx++) WriteBarrier(&inputs_[idx], inputs[idx]);

            outputs_ = (Output**)malloc(sizeof(Output*)*num_outputs);
            memset(outputs_, 0, sizeof(Output*)*num_outputs);
            for(size_t idx = 0; idx < num_outputs; idx++) WriteBarrier(&outputs_[idx], outputs[idx]);
        }
    protected:
        void Accept(WeakReferenceVisitor* vis){
            for(size_t idx = 0; idx < num_inputs_; idx++){
                if(!vis->Visit(&inputs_[idx])) break;
            }
            for(size_t idx = 0; idx < num_outputs_; idx++){
                if(!vis->Visit(&outputs_[idx])) break;
            }
        }
    public:
        ~Transaction() = default;

        std::string GetSignature() const{
            return signature_;
        }

        Handle<Input> GetInput(uint32_t idx) const{
            if(idx < 0 || idx > GetNumberOfInputs()) return nullptr;
            return inputs_[idx];
        }

        Handle<Output> GetOutput(uint32_t idx) const{
            if(idx < 0 || idx > GetNumberOfOutputs()) return nullptr;
            return outputs_[idx];
        }

        uint32_t GetNumberOfInputs() const{
            return num_inputs_;
        }

        uint32_t GetNumberOfOutputs() const{
            return num_outputs_;
        }

        uint32_t GetIndex() const{
            return index_;
        }

        uint32_t GetTimestamp() const{
            return timestamp_;
        }

        bool IsCoinbase() const{
            return GetIndex() == 0;
        }

        bool IsSigned() const{
            return !signature_.empty();
        }

        bool Sign();
        bool Accept(TransactionVisitor* visitor);
        bool WriteToMessage(RawTransaction& raw) const;
        std::string ToString() const;

        static Handle<Transaction> NewInstance(uint32_t index, Input** inputs, size_t num_inputs, Output** outputs, size_t num_outputs, uint32_t timestamp=GetCurrentTime());
        static Handle<Transaction> NewInstance(const RawTransaction& raw);
        static Handle<Transaction> NewInstance(std::fstream& fd);

        static inline Handle<Transaction> NewInstance(const std::string& filename){
            std::fstream fd(filename, std::ios::in|std::ios::binary);
            return NewInstance(fd);
        }
    };

    class TransactionVisitor{
    public:
        virtual ~TransactionVisitor() = default;

        virtual bool VisitStart(){ return true; }
        virtual bool VisitInputsStart(){ return true; }
        virtual bool VisitInput(Input* input) = 0;
        virtual bool VisitInputsEnd(){ return true; }
        virtual bool VisitOutputsStart(){ return true; }
        virtual bool VisitOutput(Output* output) = 0;
        virtual bool VisitOutputsEnd(){ return true; }
        virtual bool VisitEnd(){ return true; }
    };
}

#endif //TOKEN_TRANSACTION_H