#ifndef TOKEN_SERVICE_H
#define TOKEN_SERVICE_H

#include <grpc++/grpc++.h>
#include "service.grpc.pb.h"
#include "blockchain.h"

using namespace Token::Messages;

namespace Token{
    class TokenServiceImpl : public ::TokenService::Service{
    private:
        BlockChain chain_;
        User user_;

        static Block*
        CreateGenesis(){
            Block* block = new Block();
            Transaction* tx = block->CreateTransaction();
            tx->AddOutput("TestUser", "TestToken");
            return block;
        }
        
        inline BlockChain*
        GetBlockChain(){
            return &chain_;
        }
        
        inline Block*
        GetHeadBlock(){
            return GetBlockChain()->GetHead();
        }
        
        static inline void
        SetHeader(::BlockHeader* header, Block* block){
            header->set_height(block->GetHeight());
            header->set_hash(block->GetHash());
            header->set_merkle_root(block->GetMerkleRoot());
            for(int i = 0; i < block->GetNumberOfTransactions(); i++){
                Transaction* tx = block->GetTransactionAt(i);
                header->add_transactions(tx->GetHash());
            }
        }
    public:
        explicit TokenServiceImpl():
            user_("TestUser"),
            chain_(CreateGenesis()){}
        ~TokenServiceImpl(){}
        
        grpc::Status GetHead(grpc::ServerContext* ctx, const ::GetHeadRequest* req, ::BlockHeader* resp);
    };
}

#endif //TOKEN_SERVICE_H