#ifndef TOKEN_TRANSACTION_H
#define TOKEN_TRANSACTION_H

#include "object.h"
#include "user.h"
#include "product.h"
#include "hash.h"
#include "alloc/allocator.h"
#include "unclaimed_transaction.h"

namespace Token{
    class Input : public Object{
        friend class Transaction;
    public:
        static const intptr_t kSize = Hash::kSize
                                    + sizeof(int32_t)
                                    + User::kSize;
    private:
        Hash hash_;
        int32_t index_;
        User user_;

        Input(const Hash& tx_hash, int32_t index, const User& user):
            hash_(tx_hash),
            index_(index),
            user_(user){
            SetType(Type::kInputType);
        }
    protected:
        bool Write(const Handle<Buffer>& buff) const;
    public:
        ~Input(){}

        int32_t GetOutputIndex() const{
            return index_;
        }

        Hash GetTransactionHash() const{
            return hash_;
        }

        User GetUser() const{
            return user_;
        }

        UnclaimedTransaction* GetUnclaimedTransaction() const;
        std::string ToString() const;

        static Handle<Input> NewInstance(const Handle<Buffer>& buff);
        static Handle<Input> NewInstance(const Hash& hash, int32_t index, const User& user){
            return new Input(hash, index, user);
        }

        static Handle<Input> NewInstance(const Hash& hash, int32_t index, const std::string& user){
            return new Input(hash, index, User(user));
        }
    };

    class Output : public Object{
        friend class Transaction;
    public:
        static const intptr_t kSize = User::kSize
                                    + Product::kSize;
    private:
        User user_;
        Product product_;

        Output(const User& user, const Product& product):
            user_(user),
            product_(product){
            SetType(Type::kOutputType);
        }
    protected:
        bool Write(const Handle<Buffer>& buff) const;
    public:
        ~Output(){}

        User GetUser() const{
            return user_;
        }

        Product GetProduct() const{
            return product_;
        }

        std::string ToString() const;

        static Handle<Output> NewInstance(const Handle<Buffer>& buff);
        static Handle<Output> NewInstance(const User& user, const Product& token){
            return new Output(user, token);
        }

        static Handle<Output> NewInstance(const std::string& user, const std::string& token){
            return new Output(User(user), Product(token));
        }
    };

    struct RawTransaction{
        Timestamp timestamp_;
        intptr_t index_;
        intptr_t num_inputs_;
        Input** inputs_;
        intptr_t num_outputs_;
        Output** outputs_;
    };

    class TransactionVisitor;
    class Transaction : public BinaryObject{
        friend class Block;
        friend class TransactionMessage;
    private:
        Timestamp timestamp_;
        int64_t index_;
        int64_t num_inputs_;
        Input** inputs_;
        int64_t num_outputs_;
        Output** outputs_;
        std::string signature_;

        Transaction(Timestamp timestamp, intptr_t index, Input** inputs, intptr_t num_inputs, Output** outputs, intptr_t num_outputs):
            BinaryObject(),
            timestamp_(timestamp),
            index_(index),
            num_inputs_(num_inputs),
            inputs_(nullptr),
            num_outputs_(num_outputs),
            outputs_(nullptr),
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
#ifndef TOKEN_GCMODE_NONE
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
#endif//TOKEN_GCMODE_NONE
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

        Handle<Buffer> ToBuffer() const{
            Handle<Buffer> buff = Buffer::NewInstance(GetBufferSize());
            if(!Write(buff)){
                LOG(WARNING) << "couldn't write transaction to buffer";
                return Handle<Buffer>();
            }
            return buff;
        }

        bool Sign();
        bool Accept(TransactionVisitor* visitor);
        bool Write(const Handle<Buffer>& buff) const;
        bool Equals(const Handle<Transaction>& b) const;
        bool Compare(const Handle<Transaction>& b) const;
        intptr_t GetBufferSize() const;
        std::string ToString() const;

        static Handle<Transaction> NewInstance(const Handle<Buffer>& buffer);
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