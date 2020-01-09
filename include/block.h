#ifndef TOKEN_BLOCK_H
#define TOKEN_BLOCK_H

#include <string>
#include <sstream>

#include <vector>
#include "allocator.h"
#include "merkle.h"
#include "blockchain.pb.h"
#include "transaction.h"

namespace Token{
    class BlockVisitor{
    public:
        BlockVisitor(){}
        virtual ~BlockVisitor(){}

        virtual bool VisitBlockStart() = 0;
        virtual bool VisitTransaction(Transaction* tx) = 0;
        virtual bool VisitBlockEnd() = 0;
    };

    class Block : public AllocatorObject{
    private:
        Messages::Block raw_;

        Messages::Block*
        GetRaw(){
            return &raw_;
        }

        static inline std::string
        GetGenesisPreviousHash() {
            std::stringstream stream;
            for(int i = 0; i <= 64; i++){
                stream << "F";
            }
            return stream.str();
        }

        //TODO: Remove this constructor
        explicit Block(Messages::Block* raw):
                raw_(){
            GetRaw()->CopyFrom(*raw);
        }

        Block();
        Block(Block* parent);

        friend class TransactionPool;
        friend class BlockChain;
        friend class Message;
    public:
        Block(uint32_t height, const std::string& previous_hash);
        ~Block();

        std::string GetPreviousHash() const{
            return raw_.previous_hash();
        }

        uint32_t GetHeight() const{
            return raw_.height();
        }

        size_t GetNumberOfTransactions() const{
            return raw_.transactions_size();
        }

        bool AppendTransaction(Transaction* tx){
            Messages::Transaction* ntx = GetRaw()->add_transactions();
            ntx->CopyFrom(*tx->GetRaw());
            return true;
        }

        Transaction* GetTransactionAt(size_t idx);

        Transaction* GetCoinbaseTransaction(){
            return GetTransactionAt(0);
        }

        void Accept(BlockVisitor* vis){
            vis->VisitBlockStart();

            int i;
            for(i = 0; i < GetNumberOfTransactions(); i++){
                vis->VisitTransaction(GetTransactionAt(i));
            }

            vis->VisitBlockEnd();
        }

        bool IsGenesis(){
            return GetHeight() == 0;
        }

        bool Equals(const Messages::BlockHeader& head); //TODO: Refactor

        std::string GetHash();
        std::string GetMerkleRoot();
        std::string ToString();

        void* operator new(size_t size);

        //TODO: Remove
        friend bool operator==(const Block& lhs, const Block& rhs){
            return const_cast<Block&>(lhs).GetHash() == const_cast<Block&>(rhs).GetHash();
        }

        friend bool operator!=(const Block& lhs, const Block& rhs){
            return !operator==(lhs, rhs);
        }
    };
}

#endif //TOKEN_BLOCK_H