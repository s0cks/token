#ifndef TOKEN_BLOCK_H
#define TOKEN_BLOCK_H

#include "common.h"
#include "merkle.h"
#include "user.h"
#include "blockchain.pb.h"

namespace Token{
    class Block;

    class TransactionVisitor{
    public:
        TransactionVisitor(){}
        virtual ~TransactionVisitor() = 0;
        virtual void VisitInput(const Messages::Input& input) = 0;
        virtual void VisitOutput(const Messages::Output& output) = 0;
    };

    class Transaction : public MerkleNodeItem{
    private:
        Messages::Transaction* raw_;

        inline Messages::Transaction*
        GetRaw() const{
            return raw_;
        }

        Transaction(Messages::Transaction* raw, int index):
            raw_(raw){
            GetRaw()->set_index(index);
            GetRaw()->set_is_origin(index == 0);
        }

        friend class Block;
    public:
        void AddInput(const std::string& prev_hash, int output_idx){
            Messages::Input* input = GetRaw()->add_inputs();
            input->set_previous_hash(prev_hash);
            input->set_output_index(output_idx);
        }

        void AddOutput(const std::string& userid, std::string tokenid){
            Messages::Output* output = GetRaw()->add_outputs();
            output->set_user(userid);
        }

        const Messages::Input&
        GetInputAt(int index) const{
            return GetRaw()->inputs(index);
        }

        const Messages::Output&
        GetOutputAt(int index) const{
            return GetRaw()->outputs(index);
        }

        int GetIndex() const{
            return GetRaw()->index();
        }

        int GetNumberOfInputs() const{
            return GetRaw()->inputs().size();
        }

        int GetNumberOfOutputs() const{
            return GetRaw()->outputs().size();
        }

        bool IsOrigin() const{
            return GetRaw()->is_origin();
        }

        Byte* GetData() const;

        size_t GetDataSize() const{
            return GetRaw()->ByteSizeLong();
        }

        void Accept(TransactionVisitor* vis);
        std::string GetHash();
    };

    class BlockVisitor{
    public:
        BlockVisitor(){}
        virtual ~BlockVisitor(){}
        virtual void VisitTransaction(Transaction* tx) = 0;
    };

    class Block{
    private:
        Messages::Block* raw_;

        inline Messages::Block*
        GetRaw() const{
            return raw_;
        }

        std::string ComputeMerkleRoot();
        std::string ComputeHash();
    public:
        Block();
        Block(std::string prev_hash, int height);
        Block(Block* parent);

        std::string GetPreviousHash() const{
            return GetRaw()->prev_hash();
        }

        int GetNumberOfTransactions() const{
            return GetRaw()->transactions_size();
        }

        Transaction* GetTransactionAt(int index) const{
            return new Transaction(GetRaw()->mutable_transactions(index), index);
        }

        Transaction* GetCoinbaseTransaction() const{
            return GetTransactionAt(0);
        }

        int GetHeight() const{
            return GetRaw()->height();
        }

        Transaction* CreateTransaction();
        std::string GetHash();
        std::string GetMerkleRoot();
        void Accept(BlockVisitor* vis);
        void Write(const std::string& filename);

        friend bool operator==(Block& lhs, Block& rhs);
        friend std::ostream& operator<<(std::ostream&, const Block&);
    };
}

#endif //TOKEN_BLOCK_H