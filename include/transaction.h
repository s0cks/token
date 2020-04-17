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
        uint256_t previous_hash_;
        uint32_t index_;

        friend class Transaction;
    protected:
        bool GetBytes(CryptoPP::SecByteBlock& bytes) const;
    public:
        Input():
            previous_hash_(),
            index_(){}
        Input(const Input& other):
            previous_hash_(other.previous_hash_),
            index_(other.index_){}
        Input(const UnclaimedTransaction& utxo);
        Input(const Proto::BlockChain::Input& raw):
            previous_hash_(HashFromHexString(raw.previous_hash())),
            index_(raw.index()){}
        ~Input(){}

        uint64_t GetOutputIndex() const{
            return index_;
        }

        uint256_t GetTransactionHash() const{
            return previous_hash_;
        }

        Input& operator=(const Input& other){
            index_ = other.index_;
            previous_hash_ = other.previous_hash_;
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
        uint32_t index_;
        std::string user_;
        std::string token_;

        friend class Transaction;
    protected:
        bool GetBytes(CryptoPP::SecByteBlock& bytes) const;
    public:
        Output():
            user_(),
            token_(){}
        Output(const std::string& user, const std::string& token):
            user_(user),
            token_(token){}
        Output(const Proto::BlockChain::Output& output): Output(output.user(), output.token()){}
        Output(const Output& other): Output(other.user_, other.token_){}
        ~Output(){}

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
        typedef Proto::BlockChain::Transaction RawType;

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
            for(auto& it : raw.inputs()) inputs_.push_back(Input(it));
            for(auto& it : raw.outputs()) outputs_.push_back(Output(it));
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

    class TransactionPool : public IndexManagedPool<Transaction>{
    private:
        static TransactionPool* GetInstance();

        std::string CreateObjectLocation(const uint256_t& hash, Transaction* tx) const{
            std::string hashString = HexString(hash);
            std::string front = hashString.substr(0, 8);
            std::string tail = hashString.substr(hashString.length() - 8, hashString.length());

            std::string filename = GetRoot() + "/" + front + ".dat";
            if(FileExists(filename)){
                filename = GetRoot() + "/" + tail + ".dat";
            }
            return filename;
        }

        TransactionPool(): IndexManagedPool(FLAGS_path + "/txs"){}
    public:
        ~TransactionPool(){}

        static bool Initialize();
        static bool PutTransaction(Transaction* tx);
        static bool GetTransaction(const uint256_t& hash, Transaction* result);
        static bool RemoveTransaction(const uint256_t& hash);
    };
}

#endif //TOKEN_TRANSACTION_H