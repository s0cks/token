#ifndef TOKEN_BLOCK_CHAIN_INITIALIZER_H
#define TOKEN_BLOCK_CHAIN_INITIALIZER_H

#include "block.h"
#include "snapshot.h"

namespace Token{
    class BlockChainInitializer{
    protected:
        BlockChainInitializer() = default;

        virtual bool DoInitialization() = 0;

        static void SetGenesisNode(BlockNode* node);
        static void SetHeadNode(BlockNode* node);
    public:
        virtual ~BlockChainInitializer() = default;

        bool Initialize();
    };

    class DefaultBlockChainInitializer : public BlockChainInitializer{
    protected:
        bool DoInitialization();
    public:
        DefaultBlockChainInitializer():
            BlockChainInitializer(){}
        ~DefaultBlockChainInitializer(){}
    };

    class SnapshotBlockChainInitializer : public BlockChainInitializer{
    private:
        Snapshot* snapshot_;
    protected:
        bool DoInitialization(){
            LOG(WARNING) << "SnapshotBlockChainInitializer::DoInitialization() not implemented yet";
            return false;
        }
    public:
        SnapshotBlockChainInitializer(Snapshot* snapshot):
            BlockChainInitializer(),
            snapshot_(snapshot){}
        ~SnapshotBlockChainInitializer(){}

        Snapshot* GetSnapshot() const{
            return snapshot_;
        }
    };
}

#endif //TOKEN_BLOCK_CHAIN_INITIALIZER_H