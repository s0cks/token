#ifndef TOKEN_BLOCK_HANDLER_H
#define TOKEN_BLOCK_HANDLER_H

#include "block.h"
#include "transaction_pool.h"
#include "transaction_handler.h"

namespace Token{
    class BlockHandler : public BlockVisitor{
    private:
        Block* block_;
        bool clean_;
    public:
        BlockHandler(Block* block, bool clean):
            BlockVisitor(),
            block_(block),
            clean_(clean){}
        ~BlockHandler(){}

        bool ShouldRemoveTransactionFromPool() const{
            return clean_;
        }

        Block* GetBlock() const{
            return block_;
        }

        bool Visit(const Handle<Transaction>& tx){
            Hash hash = tx->GetHash();
            if(!TransactionHandler::ProcessTransaction(tx)){
                LOG(WARNING) << "couldn't process transaction: " << hash;
                return false;
            }

            if(ShouldRemoveTransactionFromPool()) TransactionPool::RemoveTransaction(hash);
            return true;
        }

        static bool ProcessBlock(Block* block, bool clean){
            BlockHandler handler(block, clean);
            return block->Accept(&handler);
        }
    };
}

#endif //TOKEN_BLOCK_HANDLER_H