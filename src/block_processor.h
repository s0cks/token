#ifndef TOKEN_BLOCK_PROCESSOR_H
#define TOKEN_BLOCK_PROCESSOR_H

#include "block.h"
#include "transaction_pool.h"
#include "transaction_handler.h"

namespace Token{
    class BlockProcessor : public BlockVisitor{
    protected:
        BlockProcessor() = default;
    public:
        virtual ~BlockProcessor() = default;
    };

    class GenesisBlockProcessor : public BlockProcessor{
    public:
        GenesisBlockProcessor() = default;
        ~GenesisBlockProcessor() = default;

        bool Visit(const Handle<Transaction>& tx){
            Hash hash = tx->GetHash();
            if(!TransactionHandler::ProcessTransaction(tx)){
                LOG(WARNING) << "couldn't process transaction: " << hash;
                return false;
            }
            return true;
        }
    };

    class DefaultBlockProcessor : public BlockProcessor{
    public:
        DefaultBlockProcessor() = default;
        ~DefaultBlockProcessor() = default;

        bool Visit(const Handle<Transaction>& tx){
            Hash hash = tx->GetHash();
            if(!TransactionHandler::ProcessTransaction(tx)){
                LOG(WARNING) << "couldn't process transaction: " << hash;
                return false;
            }

            if(!TransactionPool::RemoveTransaction(hash)){
                LOG(WARNING) << "couldn't remove transaction " << hash << " from pool.";
                return false;
            }

            return true;
        }
    };
}

#endif //TOKEN_BLOCK_PROCESSOR_H