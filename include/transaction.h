#ifndef TOKEN_TRANSACTION_H
#define TOKEN_TRANSACTION_H

#include "allocator.h"
#include "raw_object.h"
#include "object.h"
#include "pool.h"

namespace Token{
    class Input : public BinaryObject<Proto::BlockChain::Input>{
    public:
        typedef Proto::BlockChain::Input MessageType;
    private:
        uint256_t hash_;
        uint32_t index_;
        std::string user_;

        Input(const uint256_t& tx_hash, uint32_t index, const std::string& user):
            hash_(tx_hash),
            user_(user),
            index_(index){}

        friend class Transaction;
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

        bool Encode(MessageType& raw) const;
        std::string ToString() const;
        UnclaimedTransaction* GetUnclaimedTransaction() const;

        static Input* NewInstance(const uint256_t& tx_hash, uint32_t index, const std::string& user);
        static Input* NewInstance(const MessageType& raw);

        friend std::ostream& operator<<(std::ostream& stream, const Input& input){
            stream << input.ToString();
            return stream;
        }
    };

    class Output : public Object{
    public:
        typedef Proto::BlockChain::Output RawType;
    private:
        std::string user_;
        std::string token_;

        Output(const std::string& user, const std::string& token):
            user_(user),
            token_(token){}

        friend class Transaction;
    protected:
        bool Encode(RawType& raw) const;
        bool GetBytes(CryptoPP::SecByteBlock& bytes) const;
    public:
        ~Output(){}

        std::string GetUser() const{
            return user_;
        }

        std::string GetToken() const{
            return token_;
        }

        std::string ToString() const;

        static Output* NewInstance(const std::string& user, const std::string& token);
        static Output* NewInstance(const RawType raw);
    };

    class TransactionVisitor;
    class Transaction : public BinaryObject<Proto::BlockChain::Transaction>{
    public:
        typedef Proto::BlockChain::Transaction RawType;
        typedef std::vector<std::shared_ptr<Input>> InputList;
        typedef std::vector<std::shared_ptr<Output>> OutputList;
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
            outputs_(outputs){}

        friend class Block;
        friend class TransactionMessage;
        friend class IndexManagedPool<Transaction, RawType>;
        friend class TransactionPool;
        friend class TransactionVerifier;
    public:
        ~Transaction(){}

        std::string GetSignature() const{
            return signature_;
        }

        Input* GetInput(uint32_t idx) const{
            if(idx < 0 || idx > inputs_.size()) return nullptr;
            return inputs_[idx].get();
        }

        Output* GetOutput(uint32_t idx) const{
            if(idx < 0 || idx > outputs_.size()) return nullptr;
            return outputs_[idx].get();
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
        static Transaction* NewInstance(uint32_t index, InputList& inputs, OutputList& outputs, uint32_t timestamp=GetCurrentTime());
        static Transaction* NewInstance(const RawType& raw);
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

    class TransactionPool : public IndexManagedPool<Transaction, Transaction::RawType>{
    private:
        pthread_rwlock_t rwlock_;

        static TransactionPool* GetInstance();
        static bool PutTransaction(Transaction* tx);

        std::string CreateObjectLocation(const uint256_t& hash, Transaction* tx) const{
            if(ContainsObject(hash)){
                return GetObjectLocation(hash);
            }

            std::string hashString = HexString(hash);
            std::string front = hashString.substr(0, 8);
            std::string tail = hashString.substr(hashString.length() - 8, hashString.length());

            std::string filename = GetRoot() + "/" + front + ".dat";
            if(FileExists(filename)){
                filename = GetRoot() + "/" + tail + ".dat";
            }
            return filename;
        }

        TransactionPool():
                rwlock_(),
                IndexManagedPool(FLAGS_path + "/txs"){
            pthread_rwlock_init(&rwlock_, NULL);
        }

        friend class Node;
        friend class BlockHandler;
    public:
        ~TransactionPool(){}

        static bool Initialize();
        static bool AddTransaction(Transaction* tx);
        static bool HasTransaction(const uint256_t& hash);
        static bool RemoveTransaction(const uint256_t& hash);
        static bool GetTransactions(std::vector<uint256_t>& txs);
        static Transaction* GetTransaction(const uint256_t& hash);
        static uint64_t GetSize();
    };
}

#endif //TOKEN_TRANSACTION_H