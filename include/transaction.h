#ifndef TOKEN_TRANSACTION_H
#define TOKEN_TRANSACTION_H

#include <glog/logging.h>
#include "blockchain.pb.h"
#include "merkle.h"

namespace Token{
    class Block;

    class Input{
    private:
        std::string phash_;
        uint32_t index_;

        friend class Transaction;
    public:
        Input(): phash_(), index_(0){}
        Input(const std::string& previous_hash, uint32_t index):
            phash_(previous_hash),
            index_(index){}
        Input(const Messages::Input& input):
            phash_(input.previous_hash()),
            index_(input.index()){}
        Input(const Input& other): Input(other.phash_, other.index_){}
        ~Input(){}

        std::string GetPreviousHash() const{
            return phash_;
        }

        int GetIndex() const{
            return index_;
        }

        std::string GetHash() const;

        Input& operator=(const Input& other){
            phash_ = other.phash_;
            index_ = other.index_;
            return (*this);
        }

        friend bool operator==(const Input& lhs, const Input& rhs){
            return lhs.GetHash() == rhs.GetHash();
        }

        friend bool operator!=(const Input& lhs, const Input& rhs){
            return !operator==(lhs, rhs);
        }

        friend Messages::Input& operator<<(Messages::Input& stream, const Input* input){
            stream.set_previous_hash(input->phash_);
            stream.set_index(input->index_);
            return stream;
        }
    };

    class Output{
    private:
        std::string user_;
        std::string token_;

        friend class Transaction;
    public:
        Output(const std::string& user, const std::string& token):
            user_(user),
            token_(token){}
        Output(const Messages::Output& output): Output(output.user(), output.token()){}
        Output(const Output& other): Output(other.user_, other.token_){}
        ~Output(){}

        std::string GetUser() const{
            return user_;
        }

        std::string GetToken() const{
            return token_;
        }

        std::string GetHash() const;

        Output& operator=(const Output& other){
            user_ = other.user_;
            token_ = other.token_;
            return (*this);
        }

        friend bool operator==(const Output& lhs, const Output& rhs){
            return lhs.GetHash() == rhs.GetHash();
        }

        friend bool operator!=(const Output& lhs, const Output& rhs){
            return !operator==(lhs, rhs);
        }

        friend Messages::Output& operator<<(Messages::Output& stream, const Output* output){
            stream.set_user(output->GetUser());
            stream.set_token(output->GetToken());
            return stream;
        }
    };

    class TransactionVisitor;
    class Transaction{
    public:
        static const uintptr_t kInputsInitSize = 128;
        static const uintptr_t kOutputsInitSize = 128;
    private:
        void SetTimestamp();
        void SetIndex(uint32_t index);
        void SetSignature(std::string signature);

        uint64_t index_;
        std::vector<Input*> inputs_;
        std::vector<Output*> outputs_;

        friend class Block;
        friend class TransactionPool;
        friend class TransactionSigner;
        friend class TransactionVerifier;
    public:
        Transaction(uint64_t index):
            index_(index),
            inputs_(),
            outputs_(){}
        Transaction(const Messages::Transaction& tx):
            index_(tx.index()),
            inputs_(),
            outputs_(){
            for(auto& it : tx.inputs()) inputs_.push_back(new Input(it));
            for(auto& it : tx.outputs()) outputs_.push_back(new Output(it));
        }
        Transaction(const Transaction& other):
            index_(other.index_),
            inputs_(other.inputs_),
            outputs_(other.outputs_){}
        ~Transaction(){}

        Input* GetInputAt(uintptr_t idx) const{
            if(idx < 0 || idx > GetNumberOfInputs()) return nullptr;
            return new Input(*inputs_[idx]); //TODO: remove allocation?
        }

        Output* GetOutputAt(uintptr_t idx) const{
            if(idx < 0 || idx > GetNumberOfOutputs()) return nullptr;
            return new Output(*outputs_[idx]); //TODO: remove allocation?
        }

        size_t GetNumberOfInputs() const{
            return inputs_.size();
        }

        size_t GetNumberOfOutputs() const{
            return outputs_.size();
        }

        uint64_t GetIndex() const{
            return index_;
        }

        bool IsCoinbase() const{
            return GetIndex() == 0;
        }

        void Accept(TransactionVisitor* visitor);
        std::string GetSignature() const;
        std::string GetHash() const;

        Transaction& operator=(const Transaction& other){
            index_ = other.index_;
            inputs_ = std::vector<Input*>(other.inputs_);
            outputs_ = std::vector<Output*>(other.outputs_);
            return (*this);
        }

        friend bool operator==(const Transaction& lhs, const Transaction& rhs){
            return lhs.GetHash() == rhs.GetHash();
        }

        friend bool operator!=(const Transaction& lhs, const Transaction& rhs){
            return !operator==(lhs, rhs);
        }

        friend Transaction& operator<<(Transaction& stream, const Output& output){
            stream.outputs_.push_back(new Output(output)); //TODO: remove copy?
            return stream;
        }

        friend Transaction& operator<<(Transaction& stream, const Input& input){
            stream.inputs_.push_back(new Input(input)); //TODO: remove copy?
            return stream;
        }

        friend Messages::Transaction& operator<<(Messages::Transaction& stream, const Transaction* tx){
            stream.set_index(tx->GetIndex());
            //TODO: stream.set_timestamp();
            //TODO: stream.set_signature();
            uintptr_t idx;
            for(idx = 0; idx < tx->GetNumberOfInputs(); idx++){
                Messages::Input* raw = stream.add_inputs();
                (*raw) << tx->inputs_[idx];
            }
            for(idx = 0; idx < tx->GetNumberOfOutputs(); idx++){
                Messages::Output* raw = stream.add_outputs();
                (*raw) << tx->outputs_[idx];
            }
            return stream;
        }

        friend Messages::Transaction& operator<<(Messages::Transaction& stream, const Transaction& tx){
            stream.set_index(tx.GetIndex());
            //TODO: stream.set_timestamp();
            //TODO: stream.set_signature();
            uintptr_t idx;
            for(idx = 0; idx < tx.GetNumberOfInputs(); idx++){
                Messages::Input* raw = stream.add_inputs();
                (*raw) << tx.inputs_[idx];
            }
            for(idx = 0; idx < tx.GetNumberOfOutputs(); idx++){
                Messages::Output* raw = stream.add_outputs();
                (*raw) << tx.outputs_[idx];
            }
            return stream;
        }
    };

    class TransactionVisitor{
    public:
        virtual ~TransactionVisitor() = default;

        virtual bool VisitStart(){
            return true;
        }

        virtual bool VisitInputsStart(){
            return true;
        }

        virtual bool VisitInput(Input* input) = 0;

        virtual bool VisitInputsEnd(){
            return true;
        }

        virtual bool VisitOutputsStart(){
            return true;
        }

        virtual bool VisitOutput(Output* out) = 0;

        virtual bool VisitOutputsEnd(){
            return true;
        }

        virtual bool VisitEnd(){
            return true;
        }
    };

    class TransactionVerifier{
    private:
        Transaction* tx_;
    public:
        TransactionVerifier(Transaction* tx):
                tx_(tx){}
        ~TransactionVerifier(){}

        Transaction* GetTransaction(){
            return tx_;
        }

        bool VerifySignature();
    };

    class TransactionSigner{
    private:
        Transaction* tx_;
    public:
        TransactionSigner(Transaction* tx):
                tx_(tx){}
        ~TransactionSigner(){}

        Transaction* GetTransaction(){
            return tx_;
        }

        bool GetSignature(std::string* signature);
        bool Sign();
    };

    class TransactionPoolVisitor;
    class TransactionPool{
    public:
        static const size_t kTransactionPoolMaxSize = 1;
    private:
        static uint32_t counter_;

        static bool CreateBlock();
        static bool LoadTransaction(const std::string& filename, Messages::Transaction* result, bool deleteAfter=true);
        static bool SaveTransaction(const std::string& filename, Transaction* tx);

        TransactionPool(){}
    public:
        ~TransactionPool(){}

        static bool AddTransaction(Transaction* tx);
        static bool Initialize();
    };

    class TransactionPoolVisitor{
    public:
        TransactionPoolVisitor(){}
        virtual ~TransactionPoolVisitor() = default;

        virtual bool VisitStart() = 0;
        virtual bool VisitTransaction(Transaction* tx) = 0;
        virtual bool VisitEnd() = 0;
    };
}

#endif //TOKEN_TRANSACTION_H