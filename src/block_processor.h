#ifndef TOKEN_BLOCK_PROCESSOR_H
#define TOKEN_BLOCK_PROCESSOR_H

#include "block.h"
#include "transaction_processor.h"

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

        bool Visit(const TransactionPtr& tx){
            Hash hash = tx->GetHash();
            if(!TransactionHandler::ProcessTransaction(tx)){
                LOG(WARNING) << "couldn't process transaction: " << hash;
                return false;
            }
            return true;
        }
    };

    class SynchronizeBlockProcessor : public BlockProcessor{
        //TODO: rename SynchronizeBlockProcessor
    public:
        SynchronizeBlockProcessor() = default;
        ~SynchronizeBlockProcessor() = default;

        bool Visit(const TransactionPtr& tx){
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

        bool Visit(const TransactionPtr& tx){
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