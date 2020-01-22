#ifndef TOKEN_TRANSACTION_H
#define TOKEN_TRANSACTION_H

#include "blockchain.pb.h"
#include "merkle.h"

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
            raw_(raw){}
        Input(const Messages::Input& raw):
            raw_(new Messages::Input()){
            GetRaw()->CopyFrom(raw);
        }
        Input(const std::string& previous_hash, int index):
            raw_(new Messages::Input()){
            GetRaw()->set_previous_hash(previous_hash);
            GetRaw()->set_index(index);
        }
        ~Input(){}

        std::string GetPreviousHash() const{
            return GetRaw()->previous_hash();
        }

        int GetIndex() const{
            return GetRaw()->index();
        }

        std::string GetHash() const;

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
            raw_(raw){}
        Output(const Messages::Output& raw):
            raw_(new Messages::Output()){
            GetRaw()->CopyFrom(raw);
        }
        Output(const std::string& user, const std::string& token):
            raw_(new Messages::Output()){
            GetRaw()->set_user(user);
            GetRaw()->set_token(token);
        }
        ~Output(){}

        std::string GetUser() const{
            return GetRaw()->user();
        }

        std::string GetToken() const{
            return GetRaw()->token();
        }

        std::string GetHash() const;

        friend std::ostream& operator<<(std::ostream& stream, const Output& out){
            stream << "Output(" << out.GetUser() << "+" << out.GetToken() << ")";
            return stream;
        }
    };

    class TransactionVisitor;

    class Transaction : public MerkleNodeItem{
    private:
        Messages::Transaction raw_;

        Messages::Transaction*
        GetRaw(){
            return &raw_;
        }

        uint64_t GetByteSize();
        bool GetBytes(uint8_t** bytes, uint64_t size);
        void SetTimestamp();
        void SetSignature(std::string signature);

        friend class Block;
        friend class TransactionPool;
        friend class TransactionSigner;
        friend class TransactionVerifier;
        friend class BlockChainService;
        friend class TokenServiceClient;//TODO: Revoke
    public:
        Transaction():
                raw_(){
            SetTimestamp();
        }
        ~Transaction(){}

        Input* AddInput(const std::string& previous_hash, uint32_t idx){
            Messages::Input* in = GetRaw()->add_inputs();
            in->set_previous_hash(previous_hash);
            in->set_index(idx);
            return new Input(in);
        }

        void AddOutput(const std::string& user, const std::string& token){
            Messages::Output* out = GetRaw()->add_outputs();
            out->set_user(user);
            out->set_token(token);
        }

        Input* GetInputAt(int idx){
            return new Input(GetRaw()->inputs(idx));
        }

        Output* GetOutputAt(int idx){
            return new Output(GetRaw()->outputs(idx));
        }

        Output* GetOutput(const std::string& user, const std::string& token){
            for(int i = 0; i < GetRaw()->outputs_size(); i++){
                Messages::Output out = GetRaw()->outputs(i);
                if(out.user() == user && out.token() == token){
                    return new Output(out);
                }
            }
            return nullptr;
        }

        int GetNumberOfInputs(){
            return GetRaw()->inputs_size();
        }

        int GetNumberOfOutputs(){
            return GetRaw()->outputs_size();
        }

        uint32_t GetIndex(){
            return GetRaw()->index();
        }

        bool IsCoinbase(){
            return GetIndex() == 0;
        }

        void SetIndex(int idx){
            GetRaw()->set_index(idx);
        }

        void Accept(TransactionVisitor* visitor);
        std::string GetSignature();
        std::string GetHash();
        std::string ToString();
        HashArray GetHashArray();

        //TODO: Remove
        friend bool operator==(const Transaction& lhs, const Transaction& rhs){
            return const_cast<Transaction&>(lhs).GetHash() == const_cast<Transaction&>(rhs).GetHash();
        }
    };

    class TransactionVisitor{
    public:
        TransactionVisitor(){}
        virtual ~TransactionVisitor(){}

        virtual bool VisitStart() = 0;

        virtual bool VisitInputsStart() = 0;
        virtual bool VisitInput(Input* input) = 0;
        virtual bool VisitInputsEnd() = 0;

        virtual bool VisitOutputsStart() = 0;
        virtual bool VisitOutput(Output* out) = 0;
        virtual bool VisitOutputsEnd() = 0;

        virtual bool VisitEnd() = 0;
    };

    class Block;

    class TransactionPoolVisitor{
    public:
        TransactionPoolVisitor(){}
        virtual ~TransactionPoolVisitor() = default;

        virtual bool VisitStart() = 0;
        virtual bool VisitTransaction(Transaction* tx) = 0;
        virtual bool VisitEnd() = 0;
    };

    class TransactionPool{
    public:
        static const size_t kTransactionPoolMaxSize = 1;
    private:
        static uint32_t counter_;

        static Block* CreateBlock();
        static Transaction* LoadTransaction(const std::string& filename, bool deleteAfter=true);
        static bool SaveTransaction(const std::string& filename, Transaction* tx);
        static bool GetTransactions(std::vector<Transaction*>& txs, bool deleteAfter=true);

        TransactionPool(){}
    public:
        ~TransactionPool(){}

        static bool AddTransaction(Transaction* tx);
        static bool Initialize();
        static void Accept(TransactionPoolVisitor* vis);
    };
}

#endif //TOKEN_TRANSACTION_H