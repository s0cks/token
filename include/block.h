#ifndef TOKEN_BLOCK_H
#define TOKEN_BLOCK_H

#include <string>
#include <sstream>
#include <vector>
#include "bytes.h"
#include "merkle.h"

namespace Token{
    class Input{
    private:
        std::string prev_hash_;
        uint32_t index_;

        friend class Transaction;
    public:
        Input(std::string prev_hash, uint32_t idx):
                prev_hash_(prev_hash),
                index_(idx){}
        ~Input(){}

        std::string GetPreviousHash() const{
            return prev_hash_;
        }

        uint32_t GetIndex() const{
            return index_;
        }

        uint64_t GetSize() const{
            uint64_t size = 4;
            size += GetPreviousHash().size();
            return size;
        }

        void Encode(ByteBuffer* bytes);

        bool Equals(const Input* other) const{
            return GetIndex() == other->GetIndex() &&
                    GetPreviousHash() == other->GetPreviousHash();
        }

        static Input* Decode(ByteBuffer* bytes);

        friend bool operator ==(const Input& lhs, const Input& rhs){
            return lhs.Equals(&rhs);
        }

        friend std::ostream& operator<<(std::ostream& stream, const Input& in){
            stream << "Input:" << std::endl;
            stream << "\tIndex: " << in.GetIndex() << std::endl;
            stream << "\tPrevious Hash: " << in.GetPreviousHash() << std::endl;
            return stream;
        }
    };

    class Output{
    private:
        std::string token_;
        std::string user_;
        uint32_t index_;

        friend class Transaction;
    public:
        Output(std::string token, std::string user, uint32_t index):
            token_(token),
            user_(user),
            index_(index){
        }
        ~Output(){}

        std::string GetToken() const{
            return token_;
        }

        std::string GetUser() const{
            return user_;
        }

        uint32_t GetIndex() const{
            return index_;
        }

        uint64_t GetSize() const{
            uint64_t size = 4;
            size += GetToken().size();
            size += GetUser().size();
            return size;
        }

        void Encode(ByteBuffer* bb);

        static Output* Decode(ByteBuffer* bb);

        bool Equals(const Output* other) const{
            return GetIndex() == other->GetIndex() &&
                    GetUser() == other->GetUser() &&
                    GetToken() == other->GetToken();
        }

        friend bool operator==(const Output& lhs, const Output& rhs){
            return lhs.Equals(&rhs);
        }

        friend std::ostream& operator<<(std::ostream& stream, const Output& out){
            stream << "Output:" << std::endl;
            stream << "\tIndex: " << out.GetIndex() << std::endl;
            stream << "\tUser: " << out.GetUser() << std::endl;
            stream << "\tToken: " << out.GetToken() << std::endl;
            return stream;
        }
    };

    class TransactionVisitor;

    class Transaction : public MerkleNodeItem{
    private:
        uint32_t index_;
        std::vector<Input*> inputs_;
        std::vector<Output*> outputs_;

        void AddInput(Input* in){
            inputs_.push_back(in);
        }

        void AddOutput(Output* out){
            outputs_.push_back(out);
        }
    public:
        Transaction(uint32_t index):
            index_(index),
            inputs_(),
            outputs_(){

        }
        ~Transaction(){
            if(!inputs_.empty()){
                for(auto& it : inputs_){
                    delete it;
                }
                inputs_.clear();
            }
            if(!outputs_.empty()){
                for(auto& it : outputs_){
                    delete it;
                }
                outputs_.clear();
            }
        }

        void AddInput(const std::string& prev_hash, uint32_t idx){
            Input* in = new Input(prev_hash, idx);
            inputs_.push_back(in);
        }

        void AddOutput(const std::string& user, const std::string& token){
            Output* out = new Output(user, token, 0);
            outputs_.push_back(out);
        }

        Input* GetInputAt(int idx) const{
            return inputs_[idx];
        }

        Output* GetOutputAt(int idx) const{
            return outputs_[idx];
        }

        size_t GetNumberOfInputs() const{
            return inputs_.size();
        }

        size_t GetNumberOfOutputs() const{
            return outputs_.size();
        }

        bool IsOrigin() const{
            return GetIndex() == 0;
        }

        uint32_t GetIndex() const{
            return index_;
        }

        void Encode(ByteBuffer* bytes) const;
        void Accept(TransactionVisitor* vis) const;

        std::string GetHash() const;
        HashArray GetHashArray() const;

        friend bool operator==(const Transaction& lhs, const Transaction& rhs){
            return lhs.GetHash() == rhs.GetHash();
        }

        friend std::ostream& operator<<(std::ostream& stream, const Transaction& tx){
            stream << "Transaction" << std::endl;
            stream << "\tIndex: " << tx.GetIndex() << std::endl;
            stream << "\tInputs (" << tx.GetNumberOfInputs() << "):" << std::endl;
            int idx;
            for(idx = 0; idx < tx.GetNumberOfInputs(); idx++){
                stream << (*tx.inputs_[idx]) << std::endl;
            }

            stream << "\tOutputs (" << tx.GetNumberOfOutputs() << "):" << std::endl;
            for(idx = 0; idx < tx.GetNumberOfOutputs(); idx++){
                stream << (*tx.outputs_[idx]) << std::endl;
            }
            return stream;
        }

        static Transaction* Decode(ByteBuffer* bb);
    };

    class TransactionVisitor{
    public:
        TransactionVisitor(){}
        virtual ~TransactionVisitor(){}
        virtual void VisitInput(Input* in){}
        virtual void VisitOutput(Output* out){}
    };

    class Block{
    private:
        std::string prev_hash_;
        uint32_t height_;
        std::vector<Transaction*> transactions_;

        void AddTransaction(Transaction* tx){
            transactions_.push_back(tx);
        }

        static inline std::string
        GetGenesisPreviousHash() {
            std::stringstream stream;
            for(int i = 0; i <= 64; i++){
                stream << "F";
            }
            return stream.str();
        }
    public:
        Block():
            prev_hash_(GetGenesisPreviousHash()),
            height_(0),
            transactions_(){}
        Block(std::string prev_hash, uint32_t height):
            prev_hash_(prev_hash),
            height_(height),
            transactions_(){}
        Block(Block* parent):
            Block(parent->GetHash(), parent->GetHeight() + 1){}
        ~Block(){}

        std::string GetPreviousHash() const{
            return prev_hash_;
        }

        uint32_t GetHeight() const{
            return height_;
        }

        size_t GetNumberOfTransactions() const{
            return transactions_.size();
        }

        Transaction* CreateTransaction(){
            Transaction* tx = new Transaction(GetNumberOfTransactions());
            transactions_.push_back(tx);
            return tx;
        }

        Transaction* GetTransactionAt(size_t idx) const{
            return transactions_[idx];
        }

        Transaction* GetCoinbaseTransaction() const{
            return GetTransactionAt(0);
        }

        void Encode(ByteBuffer* bb) const;
        void Write(const std::string& filename);
        std::string GetHash() const;
        std::string GetMerkleRoot() const;

        friend bool operator==(const Block& lhs, const Block& rhs){
            return lhs.GetHash() == rhs.GetHash();
        }

        friend std::ostream& operator<<(std::ostream& stream, const Block& block){
            stream << "Block" << std::endl;
            stream << "\tHeight: " << block.GetHeight() << std::endl;
            stream << "\tPrevious Hash: " << block.GetPreviousHash() << std::endl;
            stream << "\tHash: " << block.GetHash() << std::endl;
            stream << "\tMerkle: " << block.GetMerkleRoot() << std::endl;
            return stream;
        }

        static Block* Decode(ByteBuffer* bb);
        static Block* Load(const std::string& filename);
    };
}

#endif //TOKEN_BLOCK_H