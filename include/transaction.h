#ifndef TOKEN_TRANSACTION_H
#define TOKEN_TRANSACTION_H

#include "blockchain.pb.h"
#include "merkle.h"
#include "bytes.h"

namespace Token{
    class Input{
    private:
        Messages::Input* raw_;

        inline Messages::Input*
        GetRaw() const{
            return raw_;
        }
    public:
        Input(Messages::Input* raw):
            raw_(raw){

        }
        Input(const Messages::Input& raw):
            raw_(new Messages::Input()){
            GetRaw()->CopyFrom(raw);
        }
        Input(const std::string& previous_hash, int index):
            raw_(new Messages::Input()){
            GetRaw()->set_previous_hash(previous_hash);
            GetRaw()->set_index(index);
        }

        std::string GetPreviousHash() const{
            return GetRaw()->previous_hash();
        }

        int GetIndex() const{
            return GetRaw()->index();
        }

        friend std::ostream& operator<<(std::ostream& stream, const Input& input){
            stream << "Input(" << input.GetPreviousHash() << "[" << input.GetIndex() << "])";
            return stream;
        }
    };

    class Output{
    private:
        Messages::Output* raw_;

        inline Messages::Output*
        GetRaw() const{
            return raw_;
        }
    public:
        Output(Messages::Output* raw):
            raw_(raw){

        }
        Output(const Messages::Output& raw):
            raw_(new Messages::Output()){
            GetRaw()->CopyFrom(raw);
        }
        Output(const std::string& user, const std::string& token):
            raw_(new Messages::Output()){
            GetRaw()->set_user(user);
            GetRaw()->set_token(token);
        }

        std::string GetUser() const{
            return GetRaw()->user();
        }

        std::string GetToken() const{
            return GetRaw()->token();
        }

        friend std::ostream& operator<<(std::ostream& stream, const Output& out){
            stream << "Output(" << out.GetUser() << "+" << out.GetToken() << ")";
            return stream;
        }
    };

    class TransactionVisitor;

    class Transaction : public MerkleNodeItem{
    private:
        Messages::Transaction* raw_;

        Messages::Transaction*
        GetRaw() const{
            return raw_;
        }

        friend class BlockChainService;
    public:
        Transaction(Messages::Transaction* raw):
            raw_(raw){
        }
        ~Transaction(){}

        Input* AddInput(const std::string& previous_hash, uint32_t idx){
            Messages::Input* in = GetRaw()->add_inputs();
            in->set_previous_hash(previous_hash);
            in->set_index(idx);
            return new Input(in);
        }

        Output* AddOutput(const std::string& token, const std::string& user){
            Messages::Output* out = GetRaw()->add_outputs();
            out->set_token(token);
            out->set_user(user);
            return new Output(out);
        }

        Input* GetInputAt(int idx) const{
            return new Input(GetRaw()->inputs(idx));
        }

        Output* GetOutputAt(int idx) const{
            return new Output(GetRaw()->outputs(idx));
        }

        int GetNumberOfInputs() const{
            return GetRaw()->inputs_size();
        }

        int GetNumberOfOutputs() const{
            return GetRaw()->outputs_size();
        }

        uint32_t GetIndex() const{
            return GetRaw()->index();
        }

        bool IsCoinbase() const{
            return GetIndex() == 0;
        }

        void Accept(TransactionVisitor* visitor) const;
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
}

#endif //TOKEN_TRANSACTION_H