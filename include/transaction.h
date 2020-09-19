#ifndef TOKEN_TRANSACTION_H
#define TOKEN_TRANSACTION_H

#include "array.h"
#include "object.h"
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
            user_(user),
            index_(index){
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
        std::string token_;

        Output(const UserID& user, const std::string& token):
            user_(user),
            token_(token){
            SetType(Type::kOutputType);
        }
    public:
        ~Output(){}

        UserID GetUser() const{
            return user_;
        }

        std::string GetToken() const{
            return token_;
        }

        bool Encode(ByteBuffer* bytes) const;
        size_t GetBufferSize() const;
        std::string ToString() const;

        static Handle<Output> NewInstance(ByteBuffer* bytes);

        static Handle<Output> NewInstance(const UserID& user, const std::string& token){
            return new Output(user, token);
        }

        static Handle<Output> NewInstance(const std::string& user, const std::string& token){
            return new Output(user, token);
        }
    };

    class TransactionVisitor;
    class Transaction : public Object{
        friend class Block;
        friend class TransactionMessage;
    private:
        Timestamp timestamp_;
        uint32_t index_;
        Array<Input>* inputs_;
        Array<Output>* outputs_;
        std::string signature_;

        Transaction(Timestamp timestamp, uint32_t index, const Handle<Array<Input>>& inputs, const Handle<Array<Output>>& outputs):
            Object(),
            timestamp_(timestamp),
            index_(index),
            inputs_(),
            outputs_(),
            signature_(){
            SetType(Type::kTransactionType);

            WriteBarrier(&inputs_, inputs);
            WriteBarrier(&outputs_, outputs);
        }
    protected:
        bool Accept(WeakObjectPointerVisitor* vis){
            if(!vis->Visit(&inputs_)){
                return false;
            }

            if(!vis->Visit(&outputs_)){
                return false;
            }
            return true;
        }
    public:
        ~Transaction() = default;

        Timestamp GetTimestamp() const{
            return timestamp_;
        }

        uint32_t GetIndex() const{
            return index_;
        }

        uint32_t GetNumberOfInputs() const{
            return inputs_->Length();
        }

        Handle<Input> GetInput(uint32_t idx) const{
            if(idx < 0 || idx > GetNumberOfInputs()) return nullptr;
            return inputs_->Get(idx);
        }

        Handle<Output> GetOutput(uint32_t idx) const{
            if(idx < 0 || idx > GetNumberOfOutputs()) return nullptr;
            return outputs_->Get(idx);
        }

        uint32_t GetNumberOfOutputs() const{
            return outputs_->Length();
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

        static Handle<Transaction> NewInstance(uint32_t index, const Handle<Array<Input>>& inputs, const Handle<Array<Output>>& outputs, Timestamp timestamp=GetCurrentTimestamp()){
            return new Transaction(timestamp, index, inputs, outputs);
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