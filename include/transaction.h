#ifndef TOKEN_TRANSACTION_H
#define TOKEN_TRANSACTION_H

#include <glog/logging.h>
#include "blockchain.pb.h"
#include "common.h"
#include "pool.h"
#include "binary_object.h"
#include "merkle.h"

namespace Token{
    class Block;
    class Transaction;
    class UnclaimedTransaction;
    class Output;

    class Input : public BinaryObject{
    private:
        Transaction* transaction_;
        UnclaimedTransaction* utxo_;

        friend class Transaction;
    protected:
        bool GetBytes(CryptoPP::SecByteBlock& bytes) const;
    public:
        Input(): transaction_(nullptr), utxo_(nullptr){}
        Input(const Input& other):
                transaction_(other.transaction_),
                utxo_(other.utxo_){}
        Input(Transaction* tx, UnclaimedTransaction* utxo):
            transaction_(tx),
            utxo_(utxo){}
        Input(Transaction* tx, const Proto::BlockChain::Input& raw){}
        ~Input(){}

        Transaction* GetTransaction() const{
            return transaction_;
        }

        uint64_t GetOutputIndex() const;
        uint256_t GetTransactionHash() const;

        Input& operator=(const Input& other){
            transaction_ = other.transaction_;
            utxo_ = other.utxo_;
            return (*this);
        }

        friend bool operator==(const Input& lhs, const Input& rhs){
            return lhs.GetHash() == rhs.GetHash();
        }

        friend bool operator!=(const Input& lhs, const Input& rhs){
            return !operator==(lhs, rhs);
        }

        friend Proto::BlockChain::Input& operator<<(Proto::BlockChain::Input& stream, const Input& input){
            stream.set_previous_hash(HexString(input.GetTransactionHash()));
            stream.set_index(input.GetOutputIndex());
            return stream;
        }
    };

    class Output : public BinaryObject{
    private:
        Transaction* transaction_;
        uint32_t index_;
        std::string user_;
        std::string token_;

        friend class Transaction;
    protected:
        bool GetBytes(CryptoPP::SecByteBlock& bytes) const;
    public:
        Output():
            transaction_(nullptr),
            user_(),
            token_(){}
        Output(Transaction* transaction, const std::string& user, const std::string& token):
            transaction_(transaction),
            user_(user),
            token_(token){}
        Output(Transaction* transaction, const Proto::BlockChain::Output& output): Output(transaction, output.user(), output.token()){}
        Output(const Output& other): Output(other.transaction_, other.user_, other.token_){}
        ~Output(){}

        Transaction* GetTransaction() const{
            return transaction_;
        }

        uint32_t GetIndex() const{
            return index_;
        }

        std::string GetUser() const{
            return user_;
        }

        std::string GetToken() const{
            return token_;
        }

        Output& operator=(const Output& other){
            user_ = other.user_;
            token_ = other.token_;
            transaction_ = other.transaction_;
            return (*this);
        }

        friend bool operator==(const Output& lhs, const Output& rhs){
            return lhs.GetHash() == rhs.GetHash();
        }

        friend bool operator!=(const Output& lhs, const Output& rhs){
            return !operator==(lhs, rhs);
        }

        friend Proto::BlockChain::Output& operator<<(Proto::BlockChain::Output& stream, const Output& output){
            stream.set_user(output.GetUser());
            stream.set_token(output.GetToken());
            return stream;
        }
    };

    class TransactionVisitor;
    class Transaction : public BinaryObject{
    private:
        uint64_t timestamp_;
        uint64_t index_;
        std::vector<Input> inputs_;
        std::vector<Output> outputs_;
        std::string signature_;

        friend class Block;
        friend class TransactionPool;
        friend class TransactionVerifier;
    protected:
        bool GetBytes(CryptoPP::SecByteBlock& bytes) const;
    public:
        Transaction():
            timestamp_(0),
            index_(0),
            inputs_(),
            outputs_(),
            signature_(""),
            BinaryObject(){}
        Transaction(uint64_t index, uint64_t timestamp=GetCurrentTime()):
            timestamp_(timestamp),
            index_(index),
            inputs_(),
            outputs_(),
            signature_(),
            BinaryObject(){}
        Transaction(const Proto::BlockChain::Transaction& raw):
            timestamp_(raw.timestamp()),
            index_(raw.index()),
            signature_(raw.signature()),
            inputs_(),
            outputs_(),
            BinaryObject(){
            for(auto& it : raw.inputs()) inputs_.push_back(Input(this, it));
            for(auto& it : raw.outputs()) outputs_.push_back(Output(this, it));
        }
        Transaction(const Transaction& other):
            timestamp_(other.timestamp_),
            signature_(other.signature_),
            index_(other.index_),
            inputs_(other.inputs_),
            outputs_(other.outputs_){}
        ~Transaction(){}

        bool GetInput(uint64_t idx, Input* result) const{
            if(idx < 0 || idx > inputs_.size()) return false;
            (*result) = inputs_[idx];
            return true;
        }

        bool GetOutput(uint64_t idx, Output* result) const{
            if(idx < 0 || idx > outputs_.size()) return false;
            (*result) = outputs_[idx];
            return true;
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

        uint64_t GetTimestamp() const{
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

        std::string GetSignature() const{
            return signature_;
        }

        Transaction& operator=(const Transaction& other){
            index_ = other.index_;
            timestamp_ = other.timestamp_;
            signature_ = other.signature_;
            inputs_ = std::vector<Input>(other.inputs_);
            outputs_ = std::vector<Output>(other.outputs_);
            return (*this);
        }

        friend bool operator==(const Transaction& lhs, const Transaction& rhs){
            return lhs.GetHash() == rhs.GetHash();
        }

        friend bool operator!=(const Transaction& lhs, const Transaction& rhs){
            return !operator==(lhs, rhs);
        }

        friend Transaction& operator<<(Transaction& stream, const Output& output){
            stream.outputs_.push_back(output);
            return stream;
        }

        friend Transaction& operator<<(Transaction& stream, const Input& input){
            stream.inputs_.push_back(input);
            return stream;
        }

        friend Proto::BlockChain::Transaction& operator<<(Proto::BlockChain::Transaction& stream, const Transaction& tx){
            stream.set_index(tx.GetIndex());
            stream.set_timestamp(tx.GetTimestamp());
            if(tx.IsSigned()) stream.set_signature(tx.GetSignature());
            uintptr_t idx;
            for(idx = 0; idx < tx.GetNumberOfInputs(); idx++){
                Proto::BlockChain::Input* raw = stream.add_inputs();
                (*raw) << tx.inputs_[idx];
            }
            for(idx = 0; idx < tx.GetNumberOfOutputs(); idx++){
                Proto::BlockChain::Output* raw = stream.add_outputs();
                (*raw) << tx.outputs_[idx];
            }
            return stream;
        }
    };

    class TransactionVisitor{
    public:
        virtual ~TransactionVisitor() = default;

        virtual bool VisitStart(){ return true; }
        virtual bool VisitInputsStart(){ return true; }
        virtual bool VisitInput(const Input& input) = 0;
        virtual bool VisitInputsEnd(){ return true; }
        virtual bool VisitOutputsStart(){ return true; }
        virtual bool VisitOutput(const Output& output) = 0;
        virtual bool VisitOutputsEnd(){ return true; }
        virtual bool VisitEnd(){ return true; }
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

    class TransactionPool : public IndexManagedPool{
    private:
        static TransactionPool* GetInstance();
        static bool LoadTransaction(const std::string& filename, Transaction* tx);
        static bool SaveTransaction(const std::string& filename, Transaction* tx);

        TransactionPool(): IndexManagedPool(FLAGS_path + "/txs"){}
    public:
        ~TransactionPool(){}

        static Transaction* GetTransaction(const uint256_t& hash);
        static Transaction* RemoveTransaction(const uint256_t& hash);
        static bool Initialize();
        static bool PutTransaction(Transaction* tx);
        static bool HasTransaction(const uint256_t& hash);
    };
}

#endif //TOKEN_TRANSACTION_H