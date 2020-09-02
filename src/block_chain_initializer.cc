#include "block_chain_initializer.h"
#include "node.h"
#include "common.h"
#include "block_pool.h"
#include "block_chain.h"
#include "configuration.h"
#include "transaction_pool.h"
#include "block_chain_index.h"
#include "unclaimed_transaction_pool.h"

namespace Token{
    void BlockChainInitializer::SetGenesisNode(const Handle<Block>& blk){
        BlockChain::SetGenesis(blk);
    }

    void BlockChainInitializer::SetHeadNode(const Handle<Block>& blk){
        BlockChain::SetHead(blk);
    }

    bool BlockChainInitializer::Initialize(){
        LOG(INFO) << "initializing block chain in directory: " << TOKEN_BLOCKCHAIN_HOME;
        BlockChain::SetState(BlockChain::State::kInitializing);
        if(!FileExists(TOKEN_BLOCKCHAIN_HOME)){
            if(!CreateDirectory(TOKEN_BLOCKCHAIN_HOME)){
                LOG(WARNING) << "couldn't initialize the block chain directory: " << TOKEN_BLOCKCHAIN_HOME;
                return false;
            }
        }

        Keychain::Initialize();
        BlockChainConfiguration::Initialize();
        BlockChainIndex::Initialize();
        BlockPool::Initialize();
        TransactionPool::Initialize();
        UnclaimedTransactionPool::Initialize();
        if(!DoInitialization()){
            LOG(WARNING) << "couldn't initialize the block chain in directory: " << TOKEN_BLOCKCHAIN_HOME;
            return false;
        }

        LOG(INFO) << "block chain initialized!";
        BlockChain::SetState(BlockChain::kInitialized);
        return true;
    }

    bool SnapshotBlockChainInitializer::DoInitialization(){
        //TODO: wipe existing block chain data
        Snapshot* snapshot = GetSnapshot();
        LOG(INFO) << "loading block chain from snapshot: " << snapshot->GetFilename();

        uint256_t head_hash = snapshot->GetHead();
        Handle<Block> block = nullptr; //TODO: snapshot->GetBlock(head_hash);
        LOG(INFO) << "loading <HEAD> block: " << block;
        while(true){
            uint256_t phash = block->GetPreviousHash();
            if(phash.IsNull()) break;

            Handle<Block> current = nullptr; //TODO: snapshot->GetBlock(phash);
            LOG(INFO) << "loading block: " << current;

            if(block->IsGenesis()) break;
            block = current;
        }
        return true;
    }

    bool DefaultBlockChainInitializer::DoInitialization(){
        if(!BlockChainIndex::HasBlockData()){
            LOG(INFO) << "generating new block chain in: " << TOKEN_BLOCKCHAIN_HOME;
            Handle<Block> genesis = Block::Genesis();
            SetGenesisNode(genesis);
            SetHeadNode(genesis);
            BlockChainIndex::PutBlockData(genesis);

            for(uint32_t idx = 0; idx < genesis->GetNumberOfTransactions(); idx++){
                Transaction* it = genesis->GetTransaction(idx);
                for(uint32_t out_idx = 0; out_idx < it->GetNumberOfOutputs(); out_idx++){
                    Handle<Output> out_it = it->GetOutput(out_idx);
                    Handle<UnclaimedTransaction> out_utxo = UnclaimedTransaction::NewInstance(it->GetHash(), out_idx, out_it->GetUser());
                    UnclaimedTransactionPool::PutUnclaimedTransaction(out_utxo);
                }
            }
            BlockChainIndex::PutReference("<HEAD>", genesis->GetHash());
            return true;
        }

        LOG(INFO) << "loading block chain data from " << TOKEN_BLOCKCHAIN_HOME << "....";
        uint256_t hash = BlockChainIndex::GetReference("<HEAD>");
        Handle<Block> block = BlockChainIndex::GetBlockData(hash);
        LOG(INFO) << "loading <HEAD> block: " << block->GetHeader();
        SetHeadNode(block);
        while(true){
            hash = block->GetPreviousHash();
            if(hash.IsNull()){
                SetGenesisNode(block);
                break;
            }

            Handle<Block> current = BlockChainIndex::GetBlockData(hash);
            LOG(INFO) << "loading block: " << block->GetHeader();
            block->SetPrevious(current.CastTo<Node>());
            current->SetNext(block.CastTo<Node>());

            if(block->IsGenesis()){
                SetGenesisNode(block);
                break;
            }
            block = current;
        }
        return true;
    }
}