#ifndef TOKEN_BLOCK_CHAIN_INITIALIZER_H
#define TOKEN_BLOCK_CHAIN_INITIALIZER_H

#include "block.h"
#include "snapshot.h"

namespace Token{
    class BlockChainInitializer{
    protected:
        BlockChainInitializer() = default;

        virtual bool DoInitialization() = 0;

        static void SetGenesisNode(const Handle<Block>& blk);
        static void SetHeadNode(const Handle<Block>& blk);
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
        bool DoInitialization();
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