#ifndef TOKEN_BLOCKCHAIN_H
#define TOKEN_BLOCKCHAIN_H

#include <map>
#include "common.h"
#include "array.h"
#include "bytes.h"
#include "block.h"
#include "tx_pool.h"
#include "user.h"
#include "utxo.h"

namespace Token{
    class BlockChainNode{
    private:
        Block* block_;
        BlockChainNode* parent_;
        Array<BlockChainNode*> children_;
        unsigned int height_;
        UnclaimedTransactionPool* utxo_pool_;

        inline void
        AddChild(BlockChainNode* node){
            children_.Add(node);
        }

        friend class BlockChain;
    public:
        BlockChainNode(BlockChainNode* parent, Block* block):
            parent_(parent),
            // utxo_pool_(new UnclaimedTransactionPool()),
            block_(block),
            height_(0),
            children_(0xA){
            if(parent_ != nullptr){
                height_ = parent->GetHeight() + 1;
                parent->AddChild(this);
            }
        }
        BlockChainNode(BlockChainNode* parent, Block* block, UnclaimedTransactionPool* utxo_pool):
            parent_(parent),
            //utxo_pool_(new UnclaimedTransactionPool(utxo_pool)),
            block_(block),
            children_(0xA){
            if(parent_ != nullptr){
                height_ = parent->GetHeight() + 1;
                parent->AddChild(this);
            }
        }

        Block* GetBlock() const{
            return block_;
        }

        Array<BlockChainNode*> GetChildren() const{
            return children_;
        }

        unsigned int GetHeight() const{
            return block_->GetHeight();
        }

        UnclaimedTransactionPool* GetUnclainedTransactionPool(){
            return utxo_pool_;
        }
    };

    class BlockChain{
    private:
        Array<BlockChainNode*>* heads_;
        BlockChainNode* head_;
        std::map<std::string, BlockChainNode*> nodes_;
        unsigned int height_;
        TransactionPool* txpool_;
        std::string root_;

        bool AppendGenesis(Block* block);

        inline BlockChainNode*
        GetBlockParent(Block* block){
            std::string target = block->GetPreviousHash();
            auto found = nodes_.find(target);
            if(found == nodes_.end()) return nullptr;
            return found->second;
        }

        BlockChain():
            heads_(new Array<BlockChainNode*>(0xA)),
            root_(),
            nodes_(),
            height_(0){}
    public:
        class Server;

        static BlockChain* GetInstance();
        static Server* GetServerInstance();

        Block* CreateBlock() const{
            return new Block(GetHead());
        }

        Block* GetBlockFromHash(const std::string& hash) const{
            return nodes_.find(hash)->second->GetBlock();
        }

        Block* GetHead() const{
            return head_->GetBlock();
        }

        UnclaimedTransactionPool* GetHeadUnclaimedTransactionPool(){
            return head_->GetUnclainedTransactionPool();
        }

        UnclaimedTransactionPool* GetUnclaimedTransactionPool(const std::string& block_hash){
            auto node = nodes_.find(block_hash);
            if(node != nodes_.end()){
                return node->second->GetUnclainedTransactionPool();
            }
            return nullptr;
        }

        Block* GetBlockAt(int height) const{
            if(height > GetHeight()) return nullptr;
            Block* head = GetHead();
            while(head && head->GetHeight() > height) head = GetBlockFromHash(head->GetPreviousHash());
            return head;
        }

        unsigned int GetHeight() const{
            return GetHead()->GetHeight();
        }

        TransactionPool* GetTransactionPool() const{
            return txpool_;
        }

        //TODO: Remove this
        void SetHead(Block* block){
            nodes_.clear();
            heads_->Clear();
            height_ = 0;
            AppendGenesis(block);
        }

        void SetRoot(const std::string& root){
            root_ = root;
        }

        std::string GetRoot(){
            return root_;
        }

        std::string GetLedgerRoot(){
            std::stringstream stream;
            stream << GetRoot() << "/ledger";
            return stream.str();
        }

        std::string GetLedgerFile(const std::string& filename){
            std::stringstream stream;
            stream << GetLedgerRoot() << filename;
            return stream.str();
        }

        std::string GetUnclaimedTransactionDatabase(){
            std::stringstream stream;
            stream << GetRoot() << "/utxos.db";
            return stream.str();
        }

        bool HasRoot(){
            return !GetRoot().empty();
        }

        bool Append(Block* block);
        bool Load(const std::string& root);
        bool Load(const std::string& root, const std::string& addr, int port);
        bool Save(const std::string& root);
        bool Save();
    };
}

#endif //TOKEN_BLOCKCHAIN_H