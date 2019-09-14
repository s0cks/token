#ifndef TOKEN_BLOCK_H
#define TOKEN_BLOCK_H

#include <string>
#include <sstream>

#include <vector>
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

    class Block{
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

        bool LoadBlockFromFile(const std::string& filename);

        friend class BlockChainService;
        friend class TokenServiceClient;
        friend class PeerSession;
        friend class PeerClient;
        friend class BlockChainServer;
    public:
        Block();
        Block(const std::string& path);
        Block(uint32_t height, const std::string& previous_hash);
        Block(Block* parent);
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

        Transaction* CreateTransaction(){
            return new Transaction(GetRaw()->add_transactions());
        }

        bool AppendTransaction(Transaction* tx){
            Messages::Transaction* ntx = GetRaw()->add_transactions();
            ntx->CopyFrom(*tx->GetRaw());
            return true;
        }

        Transaction* GetTransactionAt(size_t idx){
            return new Transaction(GetRaw()->mutable_transactions(idx));
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

        bool Equals(const Messages::BlockHeader& head); //TODO: Refactor
        std::string GetHash();
        std::string GetMerkleRoot();
        Token::Messages::Block* GetAsMessage();

        friend bool operator==(const Block& lhs, const Block& rhs){
            return const_cast<Block&>(lhs).GetHash() == const_cast<Block&>(rhs).GetHash();
        }

        friend std::ostream& operator<<(std::ostream& stream, const Block& block){
            stream << "Block" << std::endl;
            stream << "\tHeight: " << block.GetHeight() << std::endl;
            stream << "\tPrevious Hash: " << block.GetPreviousHash() << std::endl;
            stream << "\tHash: " << const_cast<Block&>(block).GetHash() << std::endl;
            stream << "\tMerkle: " << const_cast<Block&>(block).GetMerkleRoot() << std::endl;
            stream << "\tNumber of Transactions: " << block.GetNumberOfTransactions() << std::endl;
            stream << "\tTransactions:" << std::endl;
            for(int i = 0; i < block.GetNumberOfTransactions(); i++){
                std::cout << "\t  - #" << i << " " << const_cast<Block&>(block).GetTransactionAt(i)->GetHash() << std::endl;
            }
            return stream;
        }

        void Encode(ByteBuffer* bb); //TODO: Remove
        void Write(const std::string& filename); //TODO: Refactor

        static Block* Load(const std::string& filename); //TODO: Refactor

        //TODO: Refactor
        static Block* Load(Token::Messages::Block* block){
            return new Block(block);
        }

        void* operator new(size_t size);
    };
}

#endif //TOKEN_BLOCK_H