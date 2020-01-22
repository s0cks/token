#include <sstream>
#include <glog/logging.h>

#include "common.h"
#include "block_chain.h"
#include "block_validator.h"
#include "block_miner.h"

namespace Token{
    BlockChain*
    BlockChain::GetInstance(){
        static BlockChain instance;
        return &instance;
    }

    static inline std::string
    GetGenesisPreviousHash() {
        std::stringstream stream;
        for(int i = 0; i <= 64; i++){
            stream << "F";
        }
        return stream.str();
    }

    bool BlockChain::CreateGenesis(){
        //TODO: Remove
        LOG(WARNING) << "Creating genesis block...";
        LOG(WARNING) << "*** This functionality will be removed in future versions";
        LOG(WARNING) << "*** Genesis contains 128 base transactions for initialization";
        Transaction coinbase(0);
        for(size_t idx = 0; idx < 128; idx++){
            std::stringstream token;
            token << "TestToken" << idx;
            coinbase << Output("TestUser", token.str());
        }
        return AppendBlock(new Block(0, GetGenesisPreviousHash(), { new Transaction(coinbase) }));
    }

    bool BlockChain::LoadBlockChain(const std::string& path){
        //TODO: Refactor
        if(!HasHead()) return CreateGenesis(); //TODO: Remove
        uint32_t head = GetHeight();
        uint32_t height = 0;
        while(height <= head){
            LOG(INFO) << "loading block #" << height << "...";
            Block block;
            if(!GetInstance()->LoadBlock(height, &block)){
                LOG(ERROR) << "cannot load block from height: " << height;
                LOG(ERROR) << "*** fixme";
                return false;
            }
            GetInstance()->blocks_.push_back(new Block(block));
            height++;
        }
        return true;
    }

    bool BlockChain::InitializeChainState(const std::string& root){
        path_ = root;
        leveldb::Options options;
        options.create_if_missing = true;
        if(!leveldb::DB::Open(options, (root + "/state"), &state_).ok()){
            LOG(ERROR) << "couldn't load chain state from '" << (root + "/state") << "'";
            return false;
        }
        LOG(INFO) << "loaded chain state from '" << (root + "/state") << "'";
        return true;
    }

    bool BlockChain::Initialize(){
        std::string path = TOKEN_BLOCKCHAIN_HOME;
        LOG(INFO) << "initializing block chain in path: " << path << "...";
        if(!BlockChain::GetInstance()->InitializeChainState(path)){
            LOG(ERROR) << "error initializing chain state";
            return false;
        }
        std::string blks = (path + "/blocks");
        if(!FileExists(blks)){
            LOG(WARNING) << "blocks cache doesn't exist, creating...";
            if(!CreateDirectory(blks)){
                LOG(ERROR) << "cannot create blocks cache directory";
                return false;
            }
        }
        LOG(INFO) << "loading block chain....";
        if(!BlockChain::GetInstance()->LoadBlockChain(path)){
            LOG(ERROR) << "error loading block chain";
            return false;
        }
        if(!BlockMiner::Initialize()){
            LOG(ERROR) << "couldn't start miner thread";
            return false;
        }
        return true;
    }

    void BlockChain::Accept(Token::BlockChainVisitor* vis){
        vis->VisitStart();
        for(auto it : GetInstance()->blocks_){
            if(!vis->Visit(it)) return;
        }
        vis->VisitEnd();
    }

#define READ_LOCK pthread_rwlock_rdlock(&GetInstance()->lock_)
#define WRITE_LOCK pthread_rwlock_wrlock(&GetInstance()->lock_)
#define UNLOCK pthread_rwlock_unlock(&GetInstance()->lock_)

    uint32_t BlockChain::GetHeight(){
        READ_LOCK;
        uint32_t height;
        if(!GetInstance()->GetReference("Head", &height)){
            LOG(ERROR) << "cannot get <HEAD>";
            UNLOCK;
            return -1;
        }
        UNLOCK;
        return height;
    }

    Block* BlockChain::GetHead(){
        READ_LOCK;
        Block* head = GetBlock(GetHeight());
        UNLOCK;
        return head;
    }

    Block* BlockChain::GetGenesis(){
        return GetBlock(0);
    }


    bool BlockChain::AppendBlock(Token::Block* block){
        LOG(INFO) << "appending block: " << block->GetHash();
        LOG(INFO) << "checking for duplicate block...";
        if(ContainsBlock(block->GetHash())){
            LOG(ERROR) << "duplicate block found for: " << block->GetHash();
            UNLOCK;
            return false;
        }

        if(block->IsGenesis() && HasHead()){
            LOG(ERROR) << "cannot append genesis block:";
            //TODO: BlockPrinter::PrintAsError(block, true);
            UNLOCK;
            return false;
        }

        if(block->IsGenesis()){
            LOG(INFO) << "registering genesis's " << block->GetNumberOfTransactions() << " transactions...";

            int i;
            for(i = 0; i < block->GetNumberOfTransactions(); i++){
                Transaction* tx;
                if(!(tx = block->GetTransactionAt(i))){
                    LOG(ERROR) << "couldn't get transaction #" << i << " in block: " << block->GetHash();
                    return false;
                }
                LOG(INFO) << "registering " << tx->GetNumberOfOutputs() << " unclaimed transactions...";
                int j;
                for(j = 0; j < tx->GetNumberOfOutputs(); j++) {
                    UnclaimedTransaction utxo(tx->GetHash(), j, tx->GetOutputAt(j));
                    if (!UnclaimedTransactionPool::GetInstance()->AddUnclaimedTransaction(&utxo)) {
                        LOG(WARNING) << "couldn't create new unclaimed transaction: " << utxo.GetHash();
                        LOG(WARNING) << "*** Unclaimed Transaction: ";
                        LOG(WARNING) << "***   + Input: " << utxo.GetTransactionHash() << "[" << utxo.GetIndex()
                                     << "]";
                        LOG(WARNING) << "***   + Output: " << utxo.GetToken() << "(" << utxo.GetUser() << ")";
                        return false;
                    }
                    LOG(INFO) << "added new unclaimed transaction: " << utxo.GetHash();
                }
            }
        } else{
            LOG(INFO) << "finding parent block";
            Block* parent;
            if(!(parent = GetBlock(block->GetPreviousHash()))){
                LOG(ERROR) << "cannot find parent block: " << block->GetPreviousHash();
                UNLOCK;
                return false;
            }
            LOG(INFO) << "found parent: " << parent->GetHash();

            //TODO: Migrate validation logic
            BlockValidator validator;
            block->Accept(&validator);

            std::vector<Transaction*> valid = validator.GetValidTransactions();
            std::vector<Transaction*> invalid = validator.GetInvalidTransactions();
            if(valid.size() != block->GetNumberOfTransactions()){
                LOG(ERROR) << "block '" << block->GetHash() << "' is invalid";
                LOG(ERROR) << "block information:";
                //TODO: BlockPrinter::PrintAsError(block, true);
                UNLOCK;
                return false;
            }
        }

        WRITE_LOCK; //TODO: This should be here?!
        if(!GetInstance()->SaveBlock(block)){
            LOG(ERROR) << "couldn't save block: " << block->GetHash();
            UNLOCK;
            return false;
        }

        if(!GetInstance()->SetReference("Head", block->GetHeight())){
            LOG(ERROR) << "couldn't set <HEAD> reference";
            UNLOCK;
            return false;
        }

        GetInstance()->blocks_.push_back(block);
        LOG(INFO) << "new <HEAD>: " << block->GetHash();
        UNLOCK;
        return true;
    }

    bool BlockChain::HasHead(){
        uint32_t height;
        if(!GetInstance()->GetReference("Head", &height)){
            LOG(ERROR) << "couldn't get <HEAD>";
            return false;
        }
        return true;
    }

    bool BlockChain::SetReference(const std::string &ref, uint32_t height){
        std::stringstream val;
        val << height;
        leveldb::WriteOptions writeOpts;
        if(!GetState()->Put(writeOpts, ref, val.str()).ok()){
            LOG(ERROR) << "error setting reference '" << ref << "' to: " << height;
            return false;
        }
        return true;
    }

    bool BlockChain::GetReference(const std::string& ref, uint32_t* height){
        std::string val;
        leveldb::ReadOptions readOpts;
        if(!GetState()->Get(readOpts, ref, &val).ok()){
            LOG(ERROR) << "error getting reference to: " << ref;
            (*height) = -1;
            return false;
        }
        (*height) = static_cast<uint32_t>(atoll(val.c_str()));
        return true;
    }

    bool BlockChain::LoadBlock(uint32_t height, Token::Block* result){
        if(height < 0 || height > GetHeight()){
            LOG(WARNING) << "invalid height: " << height;
            return false;
        }
        Messages::Block raw;
        std::stringstream filename;
        filename << TOKEN_BLOCKCHAIN_HOME << "/blocks/blk" << height << ".dat";
        std::fstream fd(filename.str(), std::ios::binary|std::ios::in);
        if(!(raw.ParseFromIstream(&fd))){
            LOG(ERROR) << "cannot parse block file: " << filename.str();
            return false;
        }
        (*result) = Block(raw);
        return true;
    }

    bool BlockChain::SaveBlock(Token::Block* block){
        std::stringstream filename;
        filename << TOKEN_BLOCKCHAIN_HOME << "/blocks/blk" << block->GetHeight() << ".dat";

        if(FileExists(filename.str())){
            LOG(WARNING) << "block already written, won't overwrite block: " << filename.str();
            return false;
        }

        LOG(INFO) << "writing block #" << block->GetHeight();
        std::fstream fd(filename.str(), std::ios::out|std::ios::binary);

        Messages::Block raw;
        raw << (*block);
        if(!raw.SerializeToOstream(&fd)){
            LOG(ERROR) << "error writing block #" << block->GetHeight() << " to file: " << filename.str();
            LOG(ERROR) << "block information: ";
            //TODO: BlockPrinter::PrintAsError(block, true);
            return false;
        }

        return GetInstance()->SetReference(block->GetHash(), block->GetHeight());
    }

    Block* BlockChain::GetBlock(uint32_t height){
        if(height < 0 || height > GetHeight()) return nullptr;
        READ_LOCK;
        Block* block = GetInstance()->blocks_[height];
        UNLOCK;
        return block;
    }

    Block* BlockChain::GetBlock(const std::string &hash){
        if(GetHeight() == 0) return nullptr;
        READ_LOCK;
        for(size_t idx = 0; idx < GetInstance()->GetHeight(); idx++){
            Block* block = GetInstance()->blocks_[idx];
            if(block->GetHash() == hash){
                UNLOCK;
                return block;
            }
        }
        UNLOCK;
        return nullptr;
    }

    bool BlockChain::ContainsBlock(const std::string &hash){
        uint32_t height;
        if(!GetInstance()->GetReference(hash, &height)){
            LOG(ERROR) << "cannot find block: " << hash;
            return false;
        }
        return height >= 0 && height <= GetHeight();
    }

    bool BlockChain::GetBlockList(std::vector<std::string>& blocks){
        size_t nblocks = GetHeight() + 1;

        uint32_t idx;
        for(idx = 0; idx < nblocks; idx++){
            Block* blk = GetBlock(idx);
            blocks.push_back(blk->GetHash());
        }
        return blocks.size() == nblocks; //TODO: Check
    }
}