#ifndef TOKEN_TRANSACTION_H
#define TOKEN_TRANSACTION_H

#include "object.h"
#include "user_id.h"
#include "token_id.h"
#include "uint256_t.h"
#include "allocator.h"
#include "unclaimed_transaction.h"

namespace Token{
    class Input : public Object{
        friend class Transaction;
    private:
        uint256_t hash_;
        uint32_t index_;
        UserID user_;

        Input(const uint256_t& tx_hash, uint32_t index, const UserID& user):
            hash_(tx_hash),
            index_(index),
            user_(user){
            SetType(Type::kInputType);
        }
    public:
        ~Input(){}

        uint32_t GetOutputIndex() const{
            return index_;
        }

        uint256_t GetTransactionHash() const{
            return hash_;
        }

        UserID GetUser() const{
            return user_;
        }

        UnclaimedTransaction* GetUnclaimedTransaction() const;
        size_t GetBufferSize() const;
        bool Encode(ByteBuffer* bytes) const;
        std::string ToString() const;

        static Handle<Input> NewInstance(ByteBuffer* bytes);

        static Handle<Input> NewInstance(const uint256_t& hash, uint32_t index, const UserID& user){
            return new Input(hash, index, user);
        }

        static Handle<Input> NewInstance(const uint256_t& hash, uint32_t index, const std::string& user){
            return new Input(hash, index, UserID(user));
        }
    };

    class Output : public Object{
        friend class Transaction;
    private:
        UserID user_;
        TokenID token_;

        Output(const UserID& user, const TokenID& token):
            user_(user),
            token_(token){
            SetType(Type::kOutputType);
        }
    public:
        ~Output(){}

        UserID GetUser() const{
            return user_;
        }

        TokenID GetToken() const{
            return token_;
        }

        bool Encode(ByteBuffer* bytes) const;
        size_t GetBufferSize() const;
        std::string ToString() const;

        static Handle<Output> NewInstance(ByteBuffer* bytes);

        static Handle<Output> NewInstance(const UserID& user, const TokenID& token){
            return new Output(user, token);
        }

        static Handle<Output> NewInstance(const std::string& user, const std::string& token){
            return new Output(UserID(user), TokenID(token));
        }
    };

    class TransactionVisitor;
    class Transaction : public Object{
        friend class Block;
        friend class TransactionMessage;
    private:
        Timestamp timestamp_;
        intptr_t index_;
        Input** inputs_;
        intptr_t num_inputs_;
        Output** outputs_;
        intptr_t num_outputs_;
        std::string signature_;

        Transaction(Timestamp timestamp, intptr_t index, Input** inputs, intptr_t num_inputs, Output** outputs, intptr_t num_outputs):
                Object(),
                timestamp_(timestamp),
                index_(index),
                inputs_(nullptr),
                num_inputs_(num_inputs),
                outputs_(nullptr),
                num_outputs_(num_outputs),
                signature_(){
            SetType(Type::kTransactionType);

            inputs_ = (Input**)malloc(sizeof(Input*)*num_inputs);
            memset(inputs_, 0, sizeof(Input*)*num_inputs);
            for(intptr_t idx = 0; idx < num_inputs; idx++)
                WriteBarrier(&inputs_[idx], inputs[idx]);

            outputs_ = (Output**)malloc(sizeof(Output*)*num_outputs);
            memset(outputs_, 0, sizeof(Output*)*num_outputs);
            for(intptr_t idx = 0; idx < num_outputs; idx++)
                WriteBarrier(&outputs_[idx], outputs[idx]);
        }
    protected:
        bool Accept(WeakObjectPointerVisitor* vis){
            for(intptr_t idx = 0; idx < num_inputs_; idx++){
                if(!vis->Visit(&inputs_[idx]))
                    return false;
            }
            for(intptr_t idx = 0; idx < num_outputs_; idx++){
                if(!vis->Visit(&outputs_[idx]))
                    return false;
            }
            return true;
        }
    public:
        ~Transaction() = default;

        Timestamp GetTimestamp() const{
            return timestamp_;
        }

        intptr_t GetIndex() const{
            return index_;
        }

        intptr_t GetNumberOfInputs() const{
            return num_inputs_;
        }

        Handle<Input> GetInput(intptr_t idx) const{
            if(idx < 0 || idx > GetNumberOfInputs())
                return nullptr;
            return inputs_[idx];
        }

        Handle<Output> GetOutput(intptr_t idx) const{
            if(idx < 0 || idx > GetNumberOfOutputs())
                return nullptr;
            return outputs_[idx];
        }

        intptr_t GetNumberOfOutputs() const{
            return num_outputs_;
        }

        std::string GetSignature() const{
            return signature_;
        }

        bool IsSigned() const{
            return !signature_.empty();
        }

        bool IsCoinbase() const{
            return GetIndex() == 0;
        }

        bool Sign();
        bool Encode(ByteBuffer* bytes) const;
        bool Accept(TransactionVisitor* visitor);
        size_t GetBufferSize() const;
        std::string ToString() const;

        static Handle<Transaction> NewInstance(ByteBuffer* bytes);
        static Handle<Transaction> NewInstance(std::fstream& fd, size_t size);

        static Handle<Transaction> NewInstance(uint32_t index, Input** inputs, size_t num_inputs, Output** outputs, size_t num_outputs, Timestamp timestamp=GetCurrentTimestamp()){
            return new Transaction(timestamp, index, inputs, num_inputs, outputs, num_outputs);
        }

        static inline Handle<Transaction> NewInstance(const std::string& filename){
            std::fstream fd(filename, std::ios::in|std::ios::binary);
            return NewInstance(fd, GetFilesize(filename));
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