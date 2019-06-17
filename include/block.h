#ifndef TOKEN_BLOCK_H
#define TOKEN_BLOCK_H

#include <string>
#include <sstream>
#include <vector>
#include "bytes.h"
#include "merkle.h"
#include "blockchain.pb.h"

namespace Token{
    class Input{
    private:
        const Messages::Input& raw_;

        inline const Messages::Input&
        GetRaw() const{
            return raw_;
        }

        Input(const Messages::Input& raw):
            raw_(raw){}
        friend class Transaction;
    public:
        ~Input(){}

        std::string
        GetPreviousHash() const{
            return GetRaw().previous_hash();
        }

        int
        GetIndex() const{
            return GetRaw().index();
        }

        friend std::ostream& operator<<(std::ostream& stream, const Input& input){
            stream << "Input('" << input.GetPreviousHash() << "', " << input.GetIndex() << ")";
            return stream;
        }
    };

    class Output{
    private:
        const Messages::Output& raw_;

        inline const Messages::Output&
        GetRaw() const{
            return raw_;
        }

        Output(const Messages::Output& raw):
            raw_(raw){}
        friend class Transaction;
    public:
        ~Output(){}

        std::string
        GetUser() const{
            return GetRaw().user();
        }

        std::string
        GetToken() const{
            return GetRaw().token();
        }

        friend std::ostream& operator<<(std::ostream& stream, const Output& output){
            stream << "Output('" << output.GetUser() << "," << output.GetToken() << ")";
            return stream;
        }
    };

    class TransactionVisitor;

    class Transaction : public MerkleNodeItem{
    private:
        Messages::Transaction* raw_;

        Messages::Transaction*
        GetRaw(){
            return raw_;
        }

        friend class BlockChainService;
        friend class TokenServiceClient;
    public:
        Transaction(uint32_t index):
            raw_(new Messages::Transaction()){
            GetRaw()->set_index(index);
        }
        Transaction(Messages::Transaction* raw):
            raw_(raw){}
        ~Transaction(){}

        void AddInput(const std::string& prev_hash, uint32_t idx){
            Messages::Input* input = GetRaw()->add_inputs();
            input->set_previous_hash(prev_hash);
            input->set_index(idx);
        }

        void AddOutput(const std::string& user, const std::string& token){
            Messages::Output* output = GetRaw()->add_outputs();
            output->set_user(user);
            output->set_token(token);
        }

        Input* GetInputAt(int idx) const{
            return new Input(raw_->inputs(idx));
        }

        Output* GetOutputAt(int idx) const{
            return new Output(raw_->outputs(idx));
        }

        size_t GetNumberOfInputs() const{
            return raw_->inputs_size();
        }

        size_t GetNumberOfOutputs() const {
            return raw_->outputs_size();
        }

        bool IsOrigin() const {
            return GetIndex() == 0;
        }

        uint32_t GetIndex() const {
            return raw_->index();
        }

        void Accept(TransactionVisitor* vis) const;

        void Encode(ByteBuffer* bb);
        std::string GetHash();
        HashArray GetHashArray();

        friend bool operator==(const Transaction& lhs, const Transaction& rhs){
            return const_cast<Transaction&>(lhs).GetHash() == const_cast<Transaction&>(rhs).GetHash();
        }

        friend std::ostream& operator<<(std::ostream& stream, const Transaction& tx){
            stream << "Transaction" << std::endl;
            stream << "\tIndex: " << tx.GetIndex() << std::endl;
            stream << "\tInputs (" << tx.GetNumberOfInputs() << "):" << std::endl;
            int idx;
            for(idx = 0; idx < tx.GetNumberOfInputs(); idx++){
                stream << (*tx.GetInputAt(idx)) << std::endl;
            }

            stream << "\tOutputs (" << tx.GetNumberOfOutputs() << "):" << std::endl;
            for(idx = 0; idx < tx.GetNumberOfOutputs(); idx++){
                stream << (*tx.GetOutputAt(idx)) << std::endl;
            }
            return stream;
        }
    };

    class TransactionVisitor{
    public:
        TransactionVisitor(){}
        virtual ~TransactionVisitor(){}
        virtual void VisitInput(Input* in){}
        virtual void VisitOutput(Output* out){}
    };

    class BlockVisitor{
    public:
        BlockVisitor(){}
        virtual ~BlockVisitor(){}
        virtual void VisitTransaction(Transaction* tx){}
    };

    class Block{
    private:
        Messages::Block* raw_;

        Messages::Block*
        GetRaw(){
            return raw_;
        }

        static inline std::string
        GetGenesisPreviousHash() {
            std::stringstream stream;
            for(int i = 0; i <= 64; i++){
                stream << "F";
            }
            return stream.str();
        }

        explicit Block(Messages::Block* raw):
            raw_(raw){}
        friend class BlockChainService;
        friend class TokenServiceClient;
        friend class BlockMessage;
    public:
        Block(bool genesis):
            raw_(new Messages::Block()){
            GetRaw()->set_previous_hash(GetGenesisPreviousHash());
            GetRaw()->set_height(0);
        }
        Block(std::string prev_hash, uint32_t height):
            raw_(new Messages::Block()){
            GetRaw()->set_previous_hash(prev_hash);
            GetRaw()->set_height(height);
        }
        Block(Block* parent):
            Block(parent->GetHash(), parent->GetHeight() + 1){}
        ~Block(){}

        std::string GetPreviousHash() const{
            return raw_->previous_hash();
        }

        uint32_t GetHeight() const{
            return raw_->height();
        }

        size_t GetNumberOfTransactions() const{
            return raw_->transactions_size();
        }

        Transaction* CreateTransaction(){
            return new Transaction(GetRaw()->add_transactions());
        }

        Transaction* GetTransactionAt(size_t idx) const {
            return new Transaction(raw_->mutable_transactions(idx));
        }

        Transaction* GetCoinbaseTransaction() {
            return GetTransactionAt(0);
        }

        void Accept(BlockVisitor* vis){
            int i;
            for(i = 0; i < GetNumberOfTransactions(); i++){
                vis->VisitTransaction(GetTransactionAt(i));
            }
        }

        void Encode(ByteBuffer* bb);
        void Write(const std::string& filename);
        std::string GetHash();
        std::string GetMerkleRoot();

        friend bool operator==(const Block& lhs, const Block& rhs){
            return const_cast<Block&>(lhs).GetHash() == const_cast<Block&>(rhs).GetHash();
        }

        friend std::ostream& operator<<(std::ostream& stream, const Block& block){
            stream << "Block" << std::endl;
            stream << "\tHeight: " << block.GetHeight() << std::endl;
            stream << "\tPrevious Hash: " << block.GetPreviousHash() << std::endl;
            stream << "\tHash: " << const_cast<Block&>(block).GetHash() << std::endl;
            stream << "\tMerkle: " << const_cast<Block&>(block).GetMerkleRoot() << std::endl;
            return stream;
        }

        static Block* Load(const std::string& filename);
        static Block* Load(ByteBuffer* bb);
    };
}

#endif //TOKEN_BLOCK_H