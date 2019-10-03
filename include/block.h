#ifndef TOKEN_BLOCK_H
#define TOKEN_BLOCK_H

#include <string>
#include <sstream>

#include <vector>
#include "allocator.h"
#include "bytes.h"
#include "merkle.h"
#include "blockchain.pb.h"
#include "transaction.h"

namespace Token{
    class BlockVisitor{
    public:
        BlockVisitor(){}
        virtual ~BlockVisitor(){}
        virtual void VisitTransaction(Transaction* tx){}
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

        bool LoadBlockFromFile(const std::string& filename);

        //TODO: Remove this constructor
        explicit Block(Messages::Block* raw):
                raw_(){
            GetRaw()->CopyFrom(*raw);
        }

        Block();
        Block(Block* parent);

        friend class TransactionPool;
        friend class BlockChain;
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
            int i;
            for(i = 0; i < GetNumberOfTransactions(); i++){
                vis->VisitTransaction(GetTransactionAt(i));
            }
        }

        bool IsGenesis(){
            return GetHeight() == 0;
        }

        bool Equals(const Messages::BlockHeader& head); //TODO: Refactor
        std::string GetHash();
        std::string GetMerkleRoot();
        std::string ToString();
        std::string GetFilename();
        Token::Messages::Block* GetAsMessage(); //TODO: Remove
        void Encode(ByteBuffer* bb); //TODO: Remove
        void Write(const std::string& filename); //TODO: Remove

        static Block* Decode(Messages::Block* msg);

        void* operator new(size_t size);

        friend bool operator==(const Block& lhs, const Block& rhs){
            return const_cast<Block&>(lhs).GetHash() == const_cast<Block&>(rhs).GetHash();
        }

        //TODO: Remove
        friend std::ostream& operator<<(std::ostream& stream, const Block& block){
            stream << "Block" << std::endl;
            stream << "\tHeight: " << block.GetHeight() << std::endl;
            stream << "\tPrevious Hash: " << block.GetPreviousHash() << std::endl;
            stream << "\tHash: " << const_cast<Block&>(block).GetHash() << std::endl;
            stream << "\tMerkle: " << const_cast<Block&>(block).GetMerkleRoot() << std::endl;
            stream << "\tNumber of Transactions: " << block.GetNumberOfTransactions() << std::endl;
            stream << "\tTransactions:" << std::endl;
            for(int i = 0; i < block.GetNumberOfTransactions(); i++){
                Transaction* tx = const_cast<Block&>(block).GetTransactionAt(i);
                std::cout << "\t  - #" << i << " " << tx->GetHash() << std::endl;
            }
            return stream;
        }
    };
}

#endif //TOKEN_BLOCK_H