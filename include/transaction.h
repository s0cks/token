#ifndef TOKEN_TRANSACTION_H
#define TOKEN_TRANSACTION_H

#include "common.h"
#include "object.h"
#include "uint256_t.h"
#include "unclaimed_transaction.h"

namespace Token{
    class Input{
    public:
        typedef Proto::BlockChain::Input MessageType;
    private:
        uint256_t hash_;
        uint32_t index_;
        std::string user_;

        friend class Transaction;
    public:
        Input():
            hash_(),
            index_(0),
            user_(){}
        Input(const uint256_t& tx_hash, uint32_t index, const std::string& user):
            hash_(tx_hash),
            user_(user),
            index_(index){}
        Input(const MessageType& raw):
            hash_(HashFromHexString(raw.previous_hash())),
            index_(raw.index()),
            user_(raw.user()){}
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

        std::string ToString() const;
        UnclaimedTransaction* GetUnclaimedTransaction() const;

        Input& operator=(const Input& other){
            hash_ = other.hash_;
            index_ = other.index_;
            user_ = other.user_;
            return (*this);
        }

        friend bool operator==(const Input& a, const Input& b){
            if(a.index_ != b.index_) return false;
            if(a.hash_ != b.hash_) return false;
            return a.user_ == b.user_;
        }

        friend bool operator!=(const Input& a, const Input& b){
            return !operator==(a, b);
        }

        friend bool operator<(const Input& a, const Input& b){
            if(a.hash_ < b.hash_){
                return -1;
            } else if(b.hash_ < a.hash_){
                return 1;
            }
            return a.index_ < b.index_;
        }

        friend std::ostream& operator<<(std::ostream& stream, const Input& input){
            stream << input.ToString();
            return stream;
        }
    };

    class Output{
    public:
        typedef Proto::BlockChain::Output MessageType;
    private:
        std::string user_;
        std::string token_;

        friend class Transaction;
    public:
        Output():
            user_(),
            token_(){}
        Output(const std::string& user, const std::string& token):
            user_(user),
            token_(token){}
        Output(const MessageType& raw):
            user_(raw.user()),
            token_(raw.token()){}
        ~Output(){}

        std::string GetUser() const{
            return user_;
        }

        std::string GetToken() const{
            return token_;
        }

        std::string ToString() const;

        Output& operator=(const Output& other){
            user_ = other.user_;
            token_ = other.token_;
            return (*this);
        }

        friend bool operator==(const Output& a, const Output& b){
            return a.user_ == b.user_ &&
                    a.token_ == b.token_;
        }

        friend bool operator!=(const Output& a, const Output& b){
            return !operator==(a, b);
        }
    };

    class TransactionVisitor;
    class Transaction : public BinaryObject<Proto::BlockChain::Transaction>{
    public:
        typedef Proto::BlockChain::Transaction RawType;
        typedef std::vector<Input> InputList;
        typedef std::vector<Output> OutputList;
    private:
        uint32_t timestamp_;
        uint32_t index_;
        InputList inputs_;
        OutputList outputs_;
        std::string signature_;

        Transaction(uint32_t timestamp, uint32_t index, InputList& inputs, OutputList& outputs):
            timestamp_(timestamp),
            index_(index),
            inputs_(inputs),
            outputs_(outputs),
            signature_(){}

        friend class Block;
        friend class TransactionMessage;
        friend class TransactionVerifier;
    public:
        ~Transaction(){}

        std::string GetSignature() const{
            return signature_;
        }

        bool GetInput(uint32_t idx, Input* result) const{
            if(idx < 0 || idx > inputs_.size()) return false;
            (*result) = inputs_[idx];
            return true;
        }

        bool GetOutput(uint32_t idx, Output* result) const{
            if(idx < 0 || idx > outputs_.size()) return false;
            (*result) = outputs_[idx];
            return true;
        }

        uint32_t GetNumberOfInputs() const{
            return inputs_.size();
        }

        uint32_t GetNumberOfOutputs() const{
            return outputs_.size();
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

        InputList::iterator inputs_begin(){
            return inputs_.begin();
        }

        InputList::iterator inputs_end(){
            return inputs_.end();
        }

        OutputList::iterator outputs_begin(){
            return outputs_.begin();
        }

        OutputList::iterator outputs_end(){
            return outputs_.end();
        }

        bool Sign();
        bool Accept(TransactionVisitor* visitor);
        bool Encode(RawType& raw) const;
        std::string ToString() const;

        static Handle<Transaction> NewInstance(uint32_t index, InputList& inputs, OutputList& outputs, uint32_t timestamp=GetCurrentTime());
        static Handle<Transaction> NewInstance(const RawType& raw);
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