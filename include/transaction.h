#ifndef TOKEN_TRANSACTION_H
#define TOKEN_TRANSACTION_H

#include "hash.h"
#include "user.h"
#include "object.h"
#include "product.h"
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
            Object(Type::kInputType),
            hash_(tx_hash),
            index_(index),
            user_(user){}
    protected:
        bool Write(Buffer* buff) const;
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

        static Input* NewInstance(Buffer* buff);
        static Input* NewInstance(const Hash& hash, int32_t index, const User& user){
            return new Input(hash, index, user);
        }

        static Input* NewInstance(const Hash& hash, int32_t index, const std::string& user){
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
            Object(Type::kOutputType),
            user_(user),
            product_(product){}
    protected:
        bool Write(Buffer* buff) const;
    public:
        ~Output(){}

        User GetUser() const{
            return user_;
        }

        Product GetProduct() const{
            return product_;
        }

        std::string ToString() const;

        static Output* NewInstance(Buffer* buff);
        static Output* NewInstance(const User& user, const Product& token){
            return new Output(user, token);
        }

        static Output* NewInstance(const std::string& user, const std::string& token){
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
            BinaryObject(Type::kTransactionType),
            timestamp_(timestamp),
            index_(index),
            num_inputs_(num_inputs),
            inputs_(nullptr),
            num_outputs_(num_outputs),
            outputs_(nullptr),
            signature_(){

            inputs_ = (Input**)malloc(sizeof(Input*)*num_inputs);
            memset(inputs_, 0, sizeof(Input*)*num_inputs);
            for(intptr_t idx = 0; idx < num_inputs; idx++)
                inputs_[idx] = inputs[idx];

            outputs_ = (Output**)malloc(sizeof(Output*)*num_outputs);
            memset(outputs_, 0, sizeof(Output*)*num_outputs);
            for(intptr_t idx = 0; idx < num_outputs; idx++)
                outputs_[idx] = outputs[idx];
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

        Input* GetInput(intptr_t idx) const{
            if(idx < 0 || idx > GetNumberOfInputs())
                return nullptr;
            return inputs_[idx];
        }

        Output* GetOutput(intptr_t idx) const{
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
        bool Accept(TransactionVisitor* visitor);
        bool Write(Buffer* buff) const;
        bool Equals(Transaction* b) const;
        bool Compare(Transaction* b) const;
        intptr_t GetBufferSize() const;
        std::string ToString() const;

        static Transaction* NewInstance(Buffer* buffer);
        static Transaction* NewInstance(std::fstream& fd, size_t size);
        static Transaction* NewInstance(uint32_t index, Input** inputs, size_t num_inputs, Output** outputs, size_t num_outputs, Timestamp timestamp=GetCurrentTimestamp()){
            return new Transaction(timestamp, index, inputs, num_inputs, outputs, num_outputs);
        }

        static inline Transaction* NewInstance(const std::string& filename){
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

#define FOR_EACH_TX_POOL_STATE(V) \
    V(Uninitialized)              \
    V(Initializing)               \
    V(Initialized)

#define FOR_EACH_TX_POOL_STATUS(V) \
    V(Ok)                          \
    V(Warning)                     \
    V(Error)

    class TransactionPoolVisitor;
    class TransactionPool{
    public:
        enum State{
#define DEFINE_STATE(Name) k##Name,
            FOR_EACH_TX_POOL_STATE(DEFINE_STATE)
#undef DEFINE_STATE
        };

        friend std::ostream& operator<<(std::ostream& stream, const State& state){
            switch(state){
#define DEFINE_TOSTRING(Name) \
                case State::k##Name: \
                    stream << #Name; \
                    return stream;
                FOR_EACH_TX_POOL_STATE(DEFINE_TOSTRING)
#undef DEFINE_TOSTRING
            }
        }

        enum Status{
#define DEFINE_STATUS(Name) k##Name,
            FOR_EACH_TX_POOL_STATUS(DEFINE_STATUS)
#undef DEFINE_STATUS
        };

        friend std::ostream& operator<<(std::ostream& stream, const Status& status){
            switch(status){
#define DEFINE_TOSTRING(Name) \
                case Status::k##Name: \
                    stream << #Name; \
                    return stream;
                FOR_EACH_TX_POOL_STATUS(DEFINE_TOSTRING)
#undef DEFINE_TOSTRING

            }
        }
    private:
        TransactionPool() = delete;

        static void SetState(const State& state);
        static void SetStatus(const Status& status);
    public:
        ~TransactionPool() = delete;

        static size_t GetSize();
        static State GetState();
        static Status GetStatus();
        static std::string GetStatusMessage();
        static bool Print();
        static bool Initialize();
        static bool Accept(TransactionPoolVisitor* vis);
        static bool RemoveTransaction(const Hash& hash);
        static bool PutTransaction(const Hash& hash, Transaction* tx);
        static bool HasTransaction(const Hash& hash);
        static bool GetTransactions(std::vector<Hash>& txs);
        static Transaction* GetTransaction(const Hash& hash);

#define DEFINE_STATE_CHECK(Name) \
        static inline bool Is##Name(){ return GetState() == State::k##Name; }
        FOR_EACH_TX_POOL_STATE(DEFINE_STATE_CHECK)
#undef DEFINE_STATE_CHECK

#define DEFINE_STATUS_CHECK(Name) \
        static inline bool Is##Name(){ return GetStatus() == Status::k##Name; }
        FOR_EACH_TX_POOL_STATUS(DEFINE_STATUS_CHECK)
#undef DEFINE_STATUS_CHECK
    };

    class TransactionPoolVisitor{
    protected:
        TransactionPoolVisitor() = default;
    public:
        virtual ~TransactionPoolVisitor() = default;
        virtual bool VisitStart() const{ return true; }
        virtual bool Visit(Transaction* tx) = 0;
        virtual bool VisitEnd() const{ return true; }
    };
}

#endif //TOKEN_TRANSACTION_H